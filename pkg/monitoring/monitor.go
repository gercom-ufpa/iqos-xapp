package monitoring

import (
	"context"

	e2api "github.com/onosproject/onos-api/go/onos/e2t/e2/v1beta1"
	topoapi "github.com/onosproject/onos-api/go/onos/topo"
	e2sm_rsm "github.com/onosproject/onos-e2-sm/servicemodels/e2sm_rsm/v1/e2sm-rsm-ies"
	"github.com/onosproject/onos-lib-go/pkg/logging"
	"google.golang.org/protobuf/proto"
)

var log = logging.GetLogger("iqos-xapp", "monitoring")

// Return a new Monitor
func NewMonitor(config Config) *Monitor {
	// set log level
	log.SetLevel(logging.DebugLevel)

	return &Monitor{
		streamReader:           config.StreamReader,
		appConfig:              config.AppConfig,
		node:                   config.Node,
		nodeID:                 config.NodeID,
		rnibClient:             config.RnibClient,
		uenibClient:            config.UeClient,
		ricIndEventTriggerType: config.EventTriggerType,
	}
}

// Starts monitoring
func (m *Monitor) Start(ctx context.Context) error {
	errCh := make(chan error) // error control channel
	go func() {
		for {
			indMsg, err := m.streamReader.Recv(ctx) // receives indication msg from stream
			if err != nil {
				errCh <- err
			}
			err = m.processIndication(ctx, indMsg, m.nodeID) // process msg
			if err != nil {
				errCh <- err
			}
		}
	}()

	select { // checks error control channel
	case err := <-errCh:
		return err
	case <-ctx.Done():
		return ctx.Err()
	}
}

// Process Indication received from stream
func (m *Monitor) processIndication(ctx context.Context, indMsg e2api.Indication, nodeID topoapi.ID) error {
	indHeader := e2sm_rsm.E2SmRsmIndicationHeader{}   // gets indication header format
	indPayload := e2sm_rsm.E2SmRsmIndicationMessage{} // gets indication msg format

	err := proto.Unmarshal(indMsg.Header, &indHeader) // unmarchal header
	if err != nil {
		return err
	}

	err = proto.Unmarshal(indMsg.Payload, &indPayload) // unmarchal payload
	if err != nil {
		return err
	}

	if indPayload.GetIndicationMessageFormat1() != nil { // process format 1
		err = m.processMetricTypeMessage(ctx, indHeader.GetIndicationHeaderFormat1(), indPayload.GetIndicationMessageFormat1())
		if err != nil {
			return err
		}
	}

	if indPayload.GetIndicationMessageFormat2() != nil { // process format 2
		err = m.processEmmEventMessage(ctx, indHeader.GetIndicationHeaderFormat1(), indPayload.GetIndicationMessageFormat2(), string(nodeID))
		if err != nil {
			return err
		}
	}

	return nil
}

// Process indication message on format 1
func (m *Monitor) processMetricTypeMessage(_ context.Context, indHdr *e2sm_rsm.E2SmRsmIndicationHeaderFormat1, indMsg *e2sm_rsm.E2SmRsmIndicationMessageFormat1) error {
	// TODO: use it to ensure elastic slice
	log.Debugf("Received indication message (Metric) hdr: %v / msg: %v", indHdr, indMsg)

	return nil
}

// Process indication message on format 2
func (m *Monitor) processEmmEventMessage(ctx context.Context, indHdr *e2sm_rsm.E2SmRsmIndicationHeaderFormat1, indMsg *e2sm_rsm.E2SmRsmIndicationMessageFormat2, cuNodeID string) error {
	log.Debugf("Received indication message (EMM) hdr: %v / msg: %v", indHdr, indMsg)

	return nil
}
