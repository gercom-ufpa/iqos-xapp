package e2

import (
	"context"
	"fmt"
	"strings"

	"github.com/atomix/atomix/api/errors"
	"github.com/gercom-ufpa/iqos-xapp/pkg/broker"
	prototypes "github.com/gogo/protobuf/types"
	e2api "github.com/onosproject/onos-api/go/onos/e2t/e2/v1beta1"
	topoapi "github.com/onosproject/onos-api/go/onos/topo"
	"github.com/onosproject/onos-e2-sm/servicemodels/e2sm_rsm/pdubuilder"
	e2sm_rsm "github.com/onosproject/onos-e2-sm/servicemodels/e2sm_rsm/v1/e2sm-rsm-ies"
	"github.com/onosproject/onos-lib-go/pkg/logging"
	e2client "github.com/onosproject/onos-ric-sdk-go/pkg/e2/v1beta1"
	"google.golang.org/protobuf/proto"
)

// log initialize
var log = logging.GetLogger("iqos-xapp", "e2")

func NewManager(config Config) (Manager, error) {
	log.Info("Starting E2 Manager")

	// creates a E2 Client
	e2Client := e2client.NewClient(
		e2client.WithAppID(e2client.AppID(config.AppID)),
		e2client.WithE2TAddress( // sets E2T address
			config.E2tAddress,
			config.E2tPort,
		),
		e2client.WithServiceModel( // sets Service Model
			e2client.ServiceModelName(config.ServiceModels.RsmSMName),
			e2client.ServiceModelVersion(config.ServiceModels.RsmSMVersion),
		),
		e2client.WithEncoding(e2client.ProtoEncoding), // sets enconding
	)

	return Manager{
		appID:           config.AppID,
		e2Client:        e2Client,
		rnibClient:      config.RnibClient,
		uenibClient:     config.UenibClient,
		ServiceModels:   config.ServiceModels,
		streams:         config.Broker,
		appConfig:       config.AppConfig,
		SlicingCtrlMsgs: config.SlicingCtrlMsgs,
	}, nil
}

// Starts E2 Manager
func (m *Manager) Start() error {
	log.Info("Starting E2 Manager")
	go func() {
		ctx, cancel := context.WithCancel(context.Background()) // creates the E2 context with cancel
		defer cancel()                                          // cancel
		err := m.watchE2Connections(ctx)
		if err != nil {
			return
		}
	}()
	return nil
}

// Watch E2 connections/changes on the topology
func (m *Manager) watchE2Connections(ctx context.Context) error {
	ch := make(chan topoapi.Event)                  // topology event channel
	err := m.rnibClient.WatchE2Connections(ctx, ch) // write changes with control filter in the channel (E2T <-> E2Node)
	if err != nil {
		log.Warn(err)
		return err
	}

	for topoEvent := range ch {
		log.Debugf("Received topo event: type %v, message %v", topoEvent.Type, topoEvent)
		switch topoEvent.Type {
		case topoapi.EventType_ADDED, topoapi.EventType_NONE: // Added or existing E2Node
			relation := topoEvent.Object.Obj.(*topoapi.Object_Relation) // gets relation
			e2NodeID := relation.Relation.TgtEntityID                   // get target ID (E2NodeID) by relation
			// check if E2Node has RSM Function/SM
			if !m.rnibClient.HasRANFunction(ctx, e2NodeID, smRsmOID) {
				log.Debugf("Received E2Node does not have RSM RAN function - %v", topoEvent)
				continue
			}

			log.Debugf("New E2NodeID %v connected", e2NodeID)

			// gets supported slice configurations
			rsmSupportedCfgs, err := m.rnibClient.GetSupportedSlicingConfigTypes(ctx, e2NodeID)
			if err != nil {
				log.Warn(err)
				return err
			}

			log.Debugf("RSM supported configs: %v", rsmSupportedCfgs)
			for _, cfg := range rsmSupportedCfgs { // for each available config
				switch cfg.SlicingConfigType { // identifies its type
				case topoapi.E2SmRsmCommand_E2_SM_RSM_COMMAND_EVENT_TRIGGERS: // support event triggers
					go func() {
						// creates subscription
						err := m.createSubscription(ctx, e2NodeID, e2sm_rsm.RsmRicindicationTriggerType_RSM_RICINDICATION_TRIGGER_TYPE_UPON_EMM_EVENT)
						if err != nil {
							log.Warn(err)
						}
					}()
				}
			}
		}
	}
	return nil
}

// Creates an subscription to E2 Node
func (m *Manager) createSubscription(ctx context.Context, e2nodeID topoapi.ID, eventTrigger e2sm_rsm.RsmRicindicationTriggerType) error {
	log.Info("Creating subscription for E2 node ID with: ", e2nodeID)
	eventTriggerData, err := m.createRsmEventTrigger(eventTrigger) // creates an event trigger
	if err != nil {
		log.Warn(err)
		return err
	}

	// gets E2 Node aspects
	aspects, err := m.rnibClient.GetE2NodeAspects(ctx, e2nodeID)
	if err != nil {
		log.Warn(err)
		return err
	}

	// check if the RSM RAN Function are availables
	_, err = m.getRsmRanFunction(aspects.ServiceModels)
	if err != nil {
		log.Warn(err)
		return err
	}

	ch := make(chan e2api.Indication)                                                // channel to received E2 Indications
	e2Node := m.e2Client.Node(e2client.NodeID(e2nodeID))                             // gets E2 Node by its ID
	subName := fmt.Sprintf("%s-subscription-%s-%s", m.appID, e2nodeID, eventTrigger) // subscription name
	subSpec := e2api.SubscriptionSpec{                                               // subscription specification
		EventTrigger: e2api.EventTrigger{
			Payload: eventTriggerData, // event trigger
		},
		Actions: m.createSubscriptionActions(), // defined actions
	}
	log.Debugf("subSpec: %v", subSpec) // show subscription specs

	// subscribe on E2 Node
	channelID, err := e2Node.Subscribe(ctx, subName, subSpec, ch)
	if err != nil {
		log.Warn(err)
		return err
	}

	// creates a new stream reader to subscription
	streamReader, err := m.streams.OpenReader(ctx, e2Node, subName, channelID, subSpec)
	if err != nil {
		log.Warn(err)
		return err
	}

	// sends indications to stream
	go m.sendIndicationOnStream(streamReader.StreamID(), ch)

	// TODO: write the monitor to handle stream information
	return nil
}

// Creates an RSM Event Trigger
func (m *Manager) createRsmEventTrigger(triggerType e2sm_rsm.RsmRicindicationTriggerType) ([]byte, error) {
	// gets event trigger from PDU
	eventTriggerDef, err := pdubuilder.CreateE2SmRsmEventTriggerDefinitionFormat1(triggerType)
	if err != nil {
		log.Warn(err)
		return nil, err
	}

	// convert to bytes
	protoBytes, err := proto.Marshal(eventTriggerDef)
	if err != nil {
		log.Warn(err)
		return nil, err
	}

	return protoBytes, nil
}

// Gets RSM RAN Function by E2 Node Service Models
func (m *Manager) getRsmRanFunction(serviceModelsInfo map[string]*topoapi.ServiceModelInfo) (*topoapi.RSMRanFunction, error) {
	for _, sm := range serviceModelsInfo { // for each SM available
		smName := strings.ToLower(sm.Name)
		if smName == string(m.ServiceModels.RsmSMName) && sm.OID == smRsmOID { // if is RSM SM
			rsmRanFunction := &topoapi.RSMRanFunction{}   // RSM RAN Func model
			for _, ranFunction := range sm.RanFunctions { // for each RAN function available
				if ranFunction.TypeUrl == ranFunction.GetTypeUrl() { // is the same URL?
					err := prototypes.UnmarshalAny(ranFunction, rsmRanFunction) // put ranFunction in rsmRanFunction
					if err != nil {
						return nil, err
					}
					return rsmRanFunction, nil
				}
			}
		}
	}
	return nil, errors.New(errors.NotFound, "cannot retrieve ran functions")

}

// Creates report type subscription actions template | return actions
func (m *Manager) createSubscriptionActions() []e2api.Action {
	actions := make([]e2api.Action, 0) // actions list
	action := &e2api.Action{
		ID:   int32(0),
		Type: e2api.ActionType_ACTION_TYPE_REPORT, // report action

		SubsequentAction: &e2api.SubsequentAction{
			Type:       e2api.SubsequentActionType_SUBSEQUENT_ACTION_TYPE_CONTINUE, // continue action
			TimeToWait: e2api.TimeToWait_TIME_TO_WAIT_ZERO,                         // without timeout
		},
	}
	actions = append(actions, *action)
	return actions
}

// Send E2 indications to stream
func (m *Manager) sendIndicationOnStream(streamID broker.StreamID, ch chan e2api.Indication) {
	streamWriter, err := m.streams.GetWriter(streamID) // gets stream writer
	if err != nil {
		log.Error(err)
		return
	}

	// for each msg in E2 indication channel
	for msg := range ch {
		err := streamWriter.Send(msg) // write indication on stream
		if err != nil {
			log.Warn(err)
			return
		}
	}
}
