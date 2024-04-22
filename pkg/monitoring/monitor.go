package monitoring

import (
	"context"
	"fmt"

	"github.com/google/uuid"
	e2api "github.com/onosproject/onos-api/go/onos/e2t/e2/v1beta1"
	topoapi "github.com/onosproject/onos-api/go/onos/topo"
	uenib_api "github.com/onosproject/onos-api/go/onos/uenib"
	e2sm_rsm "github.com/onosproject/onos-e2-sm/servicemodels/e2sm_rsm/v1/e2sm-rsm-ies"
	"github.com/onosproject/onos-lib-go/pkg/errors"
	"github.com/onosproject/onos-lib-go/pkg/logging"
	"google.golang.org/protobuf/proto"
)

var log = logging.GetLogger("iqos-xapp", "monitoring")

// Return a new Monitor
func NewMonitor(config Config) *Monitor {
	// set log level (TODO: remove me)
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
	// TODO: use it to ensure slice's elasticity
	log.Debugf("Received indication message (Metric) hdr: %v / msg: %v", indHdr, indMsg)

	return nil
}

// Process indication message on format 2
func (m *Monitor) processEmmEventMessage(ctx context.Context, indHdr *e2sm_rsm.E2SmRsmIndicationHeaderFormat1, indMsg *e2sm_rsm.E2SmRsmIndicationMessageFormat2, cuNodeID string) error {
	log.Debugf("Received indication message (EMM) hdr: %v / msg: %v", indHdr, indMsg)

	// UE's identify technology
	var CuUeF1apID, DuUeF1apID, RanUeNgapID, AmfUeNgapID int64
	var EnbUeS1apID int32
	bIDList := make([]*uenib_api.BearerId, 0)

	duNodeID, err := m.rnibClient.GetTargetDUE2NodeID(ctx, topoapi.ID(cuNodeID)) // get DU by CU ID
	log.Debugf("Cu ID %v - Du ID %v", cuNodeID, duNodeID)
	if err != nil {
		log.Warn(err)
		return err
	}

	for _, ID := range indMsg.GetUeIdlist() { // checks UE ID type
		if ID.GetCuUeF1ApId() != nil {
			CuUeF1apID = ID.GetCuUeF1ApId().GetValue()
		} else if ID.GetDuUeF1ApId() != nil {
			DuUeF1apID = ID.GetDuUeF1ApId().GetValue()
		} else if ID.GetRanUeNgapId() != nil {
			RanUeNgapID = ID.GetRanUeNgapId().GetValue()
		} else if ID.GetAmfUeNgapId() != nil {
			AmfUeNgapID = ID.GetAmfUeNgapId().GetValue()
		} else if ID.GetEnbUeS1ApId() != nil {
			EnbUeS1apID = ID.GetEnbUeS1ApId().GetValue()
		} else {
			return errors.NewNotFound("UE ID type not found!")
		}
	}

	for _, bID := range indMsg.GetBearerId() { // checks bearer data
		if bID.GetDrbId().GetFiveGdrbId() != nil {
			// for 5G
			flowMapToDrb := make([]*uenib_api.QoSflowLevelParameters, 0) // QoS param. list

			for _, fItem := range bID.GetDrbId().GetFiveGdrbId().GetFlowsMapToDrb() { // Get 5QI
				if fItem.GetNonDynamicFiveQi() != nil { // for not dinamic 5QI
					flowMapToDrb = append(flowMapToDrb, &uenib_api.QoSflowLevelParameters{
						QosFlowLevelParameters: &uenib_api.QoSflowLevelParameters_NonDynamicFiveQi{
							NonDynamicFiveQi: &uenib_api.NonDynamicFiveQi{
								FiveQi: &uenib_api.FiveQi{
									Value: fItem.GetNonDynamicFiveQi().GetFiveQi().GetValue(),
								},
							},
						},
					})
				} else if fItem.GetDynamicFiveQi() != nil { // for dinamic 5QI
					flowMapToDrb = append(flowMapToDrb, &uenib_api.QoSflowLevelParameters{
						QosFlowLevelParameters: &uenib_api.QoSflowLevelParameters_DynamicFiveQi{
							DynamicFiveQi: &uenib_api.DynamicFiveQi{
								PriorityLevel:    fItem.GetDynamicFiveQi().GetPriorityLevel(),
								PacketDelayBudge: fItem.GetDynamicFiveQi().GetPacketDelayBudget(),
								PacketErrorRate:  fItem.GetDynamicFiveQi().GetPacketErrorRate(),
							},
						},
					})
				}
			}
			uenibBID := uenib_api.BearerId{ // creates bearer ID with 5QI values
				BearerId: &uenib_api.BearerId_DrbId{
					DrbId: &uenib_api.DrbId{
						DrbId: &uenib_api.DrbId_FiveGdrbId{
							FiveGdrbId: &uenib_api.FiveGDrbId{
								Value: bID.GetDrbId().GetFiveGdrbId().GetValue(),
								Qfi: &uenib_api.Qfi{
									Value: bID.GetDrbId().GetFiveGdrbId().GetQfi().GetValue(),
								},
								FlowsMapToDrb: flowMapToDrb,
							},
						},
					},
				},
			}
			bIDList = append(bIDList, &uenibBID)
		} else if bID.GetDrbId().GetFourGdrbId() != nil {
			// for 4G
			uenibBID := uenib_api.BearerId{ // creates bearer ID with 4G values
				BearerId: &uenib_api.BearerId_DrbId{
					DrbId: &uenib_api.DrbId{
						DrbId: &uenib_api.DrbId_FourGdrbId{
							FourGdrbId: &uenib_api.FourGDrbId{
								Value: bID.GetDrbId().GetFourGdrbId().GetValue(),
								Qci: &uenib_api.Qci{
									Value: bID.GetDrbId().GetFourGdrbId().GetQci().GetValue(),
								},
							},
						},
					},
				},
			}
			bIDList = append(bIDList, &uenibBID)
		}
	}

	switch indMsg.GetTriggerType() {
	case e2sm_rsm.RsmEmmTriggerType_RSM_EMM_TRIGGER_TYPE_UE_ATTACH, e2sm_rsm.RsmEmmTriggerType_RSM_EMM_TRIGGER_TYPE_HAND_IN_UE_ATTACH:
		// UE entry into the network
		// TODO: Add logic to get GlobalUEID here after SMO is integrated - future

		rsmUE := uenib_api.RsmUeInfo{ // creates a new RSM UE
			GlobalUeID: "iqos-" + uuid.New().String(),
			UeIdList: &uenib_api.UeIdentity{
				CuUeF1apID: &uenib_api.CuUeF1ApID{
					Value: CuUeF1apID,
				},
				DuUeF1apID: &uenib_api.DuUeF1ApID{
					Value: DuUeF1apID,
				},
				RANUeNgapID: &uenib_api.RanUeNgapID{
					Value: RanUeNgapID,
				},
				AMFUeNgapID: &uenib_api.AmfUeNgapID{
					Value: AmfUeNgapID,
				},
				EnbUeS1apID: &uenib_api.EnbUeS1ApID{
					Value: EnbUeS1apID,
				},
			},
			BearerIdList: bIDList,
			CellGlobalId: indHdr.GetCgi().String(),
			CuE2NodeId:   cuNodeID,
			DuE2NodeId:   string(duNodeID),
			SliceList:    make([]*uenib_api.SliceInfo, 0),
		}
		err := m.uenibClient.AddRsmUE(ctx, &rsmUE) // stores UE into UE-NIB
		if err != nil {
			return err
		}

		log.Debugf("pushed rsmUE: %v", rsmUE)

	case e2sm_rsm.RsmEmmTriggerType_RSM_EMM_TRIGGER_TYPE_UE_DETACH, e2sm_rsm.RsmEmmTriggerType_RSM_EMM_TRIGGER_TYPE_HAND_OUT_UE_ATTACH:
		// UE exit from the network
		switch indMsg.GetPrefferedUeIdtype() { // checks UE ID type and delete it
		case e2sm_rsm.UeIdType_UE_ID_TYPE_CU_UE_F1_AP_ID:
			err := m.uenibClient.DeleteRsmUEWithPreferredID(ctx, cuNodeID, uenib_api.UeIdType_UE_ID_TYPE_CU_UE_F1_AP_ID, CuUeF1apID)
			if err != nil {
				return err
			}
		case e2sm_rsm.UeIdType_UE_ID_TYPE_DU_UE_F1_AP_ID:
			err := m.uenibClient.DeleteRsmUEWithPreferredID(ctx, cuNodeID, uenib_api.UeIdType_UE_ID_TYPE_DU_UE_F1_AP_ID, DuUeF1apID)
			if err != nil {
				return err
			}
		case e2sm_rsm.UeIdType_UE_ID_TYPE_RAN_UE_NGAP_ID:
			err := m.uenibClient.DeleteRsmUEWithPreferredID(ctx, cuNodeID, uenib_api.UeIdType_UE_ID_TYPE_RAN_UE_NGAP_ID, RanUeNgapID)
			if err != nil {
				return err
			}
		case e2sm_rsm.UeIdType_UE_ID_TYPE_AMF_UE_NGAP_ID:
			err := m.uenibClient.DeleteRsmUEWithPreferredID(ctx, cuNodeID, uenib_api.UeIdType_UE_ID_TYPE_AMF_UE_NGAP_ID, AmfUeNgapID)
			if err != nil {
				return err
			}
		case e2sm_rsm.UeIdType_UE_ID_TYPE_ENB_UE_S1_AP_ID:
			err := m.uenibClient.DeleteRsmUEWithPreferredID(ctx, cuNodeID, uenib_api.UeIdType_UE_ID_TYPE_ENB_UE_S1_AP_ID, int64(EnbUeS1apID))
			if err != nil {
				return err
			}
		default:
			return errors.NewNotSupported(fmt.Sprintf("Unknown preferred ID type: %v", indMsg.GetPrefferedUeIdtype())) // UE ID type unknown
		}
	default:
		return errors.NewNotSupported(fmt.Sprintf("Unknown EMM trigger type: %v", indMsg.GetTriggerType())) // Event trigger type unknown
	}

	return nil
}
