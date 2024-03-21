package slicing

import (
	"context"
	"fmt"
	"strconv"
	"time"

	rsmapi "github.com/onosproject/onos-api/go/onos/rsm"
	topoapi "github.com/onosproject/onos-api/go/onos/topo"
	uenib_api "github.com/onosproject/onos-api/go/onos/uenib" // used by delete func
	e2sm_rsm "github.com/onosproject/onos-e2-sm/servicemodels/e2sm_rsm/v1/e2sm-rsm-ies"
	e2sm_v2_ies "github.com/onosproject/onos-e2-sm/servicemodels/e2sm_rsm/v1/e2sm-v2-ies"
	"github.com/onosproject/onos-lib-go/pkg/logging"
	"github.com/onosproject/onos-rsm/pkg/northbound"    // TODO
	"github.com/onosproject/onos-rsm/pkg/southbound/e2" // TODO
)

var log = logging.GetLogger("iqos-xapp", "slicemgr")

func NewManager(opts ...Option) Manager { // start slicing manager with configs applyied in options.go
	log.Info("Init IQoS-xAPP Slicing Manager")
	options := Options{}

	for _, opt := range opts {
		opt.apply(&options)
	}

	return Manager{ // return a new instance of Manager configured with defined options
		rsmMsgCh:              options.Chans.RsmMsgCh, // Included configs of communnications channels to control mensages related with create, update and delete slices
		ctrlReqChsSliceCreate: options.Chans.CtrlReqChsSliceCreate,
		ctrlReqChsSliceUpdate: options.Chans.CtrlReqChsSliceUpdate,
		ctrlReqChsSliceDelete: options.Chans.CtrlReqChsSliceDelete,
		ctrlReqChsUeAssociate: options.Chans.CtrlReqChsUeAssociate, //control request channels to associate UEs in a specific slice
		rnibClient:            options.App.RnibClient,              // clients RNIB and UENIB
		uenibClient:           options.App.UenibClient,
		ctrlMsgHandler:        e2.NewControlMessageHandler(), // manipulator of control mensages
		ackTimer:              options.App.AckTimer,          // timer
	}
}

func (m *Manager) Run(ctx context.Context) {
	go m.DispatchNbiMsg(ctx) // create a dispatcher in a separated go routine not to block msgs from simulteneas entries
}

func (m *Manager) DispatchNbiMsg(ctx context.Context) {
	log.Info("Run nbi msg dispatcher")
	for msg := range m.rsmMsgCh { // receive loop msg by channel rsmMsg, this msg come from northbound api
		log.Debugf("Received message from NBI: %v", msg)
		var ack northbound.Ack
		var err error
		switch msg.Message.(type) { // process the msg based on it type and call a respective manipulation function
		case *rsmapi.CreateSliceRequest:
			err = m.handleNbiCreateSliceRequest(ctx, msg.Message.(*rsmapi.CreateSliceRequest), msg.NodeID)
		case *rsmapi.UpdateSliceRequest:
			err = m.handleNbiUpdateSliceRequest(ctx, msg.Message.(*rsmapi.UpdateSliceRequest), msg.NodeID)
		case *rsmapi.DeleteSliceRequest:
			err = m.handleNbiDeleteSliceRequest(ctx, msg.Message.(*rsmapi.DeleteSliceRequest), msg.NodeID)
		//case *rsmapi.SetUeSliceAssociationRequest:
		//	err = m.handleNbiSetUeSliceAssociationRequest(ctx, msg.Message.(*rsmapi.SetUeSliceAssociationRequest), msg.NodeID)
		default:
			err = fmt.Errorf("unknown msg type: %v", msg)
		}
		if err != nil { // error case
			ack = northbound.Ack{
				Success: false,
				Reason:  err.Error(),
			}
		} else { // sucess case
			ack = northbound.Ack{
				Success: true,
			}
		} // this logic provid a feedback mecanism to northbound's operations
		msg.AckCh <- ack
	}
}

func (m *Manager) handleNbiCreateSliceRequest(ctx context.Context, req *rsmapi.CreateSliceRequest, nodeID topoapi.ID) error {
	log.Infof("Called Create Slice: %v", req) //  log the function "create slice" was called
	sliceID, err := strconv.Atoi(req.SliceId) //  convert slice id from string to integers
	if err != nil {
		return fmt.Errorf("failed to convert slice id to int - %v", err.Error()) // error case
	}
	weightInt, err := strconv.Atoi(req.Weight) // convert weight from string to integers
	if err != nil {
		return fmt.Errorf("failed to convert weight to int - %v", err.Error()) // error case
	}
	weight := int32(weightInt) // convert int to int32

	cmdType := e2sm_rsm.E2SmRsmCommand_E2_SM_RSM_COMMAND_SLICE_CREATE // cmdType's value represents a command to create slice
	var sliceSchedulerType e2sm_rsm.SchedulerType                     // create a var of type "SchedulerType" defined in "e2sm_rsm"
	switch req.SchedulerType {                                        // start a swicth to define a Scheduler Type based on "SchedulerType" presents in "req"
	case rsmapi.SchedulerType_SCHEDULER_TYPE_ROUND_ROBIN:
		sliceSchedulerType = e2sm_rsm.SchedulerType_SCHEDULER_TYPE_ROUND_ROBIN
	case rsmapi.SchedulerType_SCHEDULER_TYPE_PROPORTIONALLY_FAIR:
		sliceSchedulerType = e2sm_rsm.SchedulerType_SCHEDULER_TYPE_PROPORTIONALLY_FAIR
	case rsmapi.SchedulerType_SCHEDULER_TYPE_QOS_BASED:
		sliceSchedulerType = e2sm_rsm.SchedulerType_SCHEDULER_TYPE_QOS_BASED
	default:
		sliceSchedulerType = e2sm_rsm.SchedulerType_SCHEDULER_TYPE_ROUND_ROBIN // default scheduler type if not a single type was indentified
	}

	var sliceType e2sm_rsm.SliceType // create a var of type "SliceType" defined in "e2sm_rsm"
	switch req.SliceType {           // start a swicth to define a Scheduler Type based on "SliceType" presents in "req"
	case rsmapi.SliceType_SLICE_TYPE_DL_SLICE:
		sliceType = e2sm_rsm.SliceType_SLICE_TYPE_DL_SLICE
	case rsmapi.SliceType_SLICE_TYPE_UL_SLICE:
		sliceType = e2sm_rsm.SliceType_SLICE_TYPE_UL_SLICE
	default:
		sliceType = e2sm_rsm.SliceType_SLICE_TYPE_DL_SLICE // default slice type if not a single type was indentified
	}

	sliceConfig := &e2sm_rsm.SliceConfig{ // create and start a instance of structure "SliceConfig" defined in e2sm_rsm
		SliceId: &e2sm_rsm.SliceId{ //instance of SliceId in e2sm_rsm
			Value: int64(sliceID), //included value of sliceID, converted into int64
		},
		SliceConfigParameters: &e2sm_rsm.SliceParameters{ // instance of SliceParameters with SchedulerType and Weight
			SchedulerType: sliceSchedulerType,
			Weight:        &weight,
		},
		SliceType: sliceType, // define slice type
	}

	ctrlMsg, err := m.ctrlMsgHandler.CreateControlRequest(cmdType, sliceConfig, nil) //  create a control request E2 based on cmdType and Slice Config
	if err != nil {
		return fmt.Errorf("failed to create the control message - %v", err.Error()) // error case
	}

	hasSliceItem := m.rnibClient.HasRsmSliceItemAspect(ctx, topoapi.ID(req.E2NodeId), req.SliceId, req.GetSliceType())
	// verify if exist a slice with specified ID and slice type in RNIB through RNIB client. topoapi.ID (req.E2NodeId) indicate ID of E2 node where verication is carried out.
	if hasSliceItem {
		return fmt.Errorf("slice ID %v already exists", sliceID) // error case, if slice already exists.
	}

	// send control message
	ackCh := make(chan e2.Ack) // create a ack channel defined in e2. These channels are use to communication between go routines
	msg := &e2.CtrlMsg{        // create a instance CtrlMsg, represents a control msgs to be send
		CtrlMsg: ctrlMsg, // it has the  control msg and ack channel
		AckCh:   ackCh,
	}
	go func() {
		m.ctrlReqChsSliceCreate[string(nodeID)] <- msg // go routine to send control msg created in "msg" to specified channel do NodeID
	}()

	// ackTimer -1 is for uenib/topo debugging and integration test, must wait an ack
	if m.ackTimer != -1 {
		var ack e2.Ack
		select { // wait events from multiple channels
		case <-time.After(time.Duration(m.ackTimer) * time.Second): // if timeout happens first, an error returns
			return fmt.Errorf("timeout happens: E2 SBI could not send ACK until timer expired")
		case ack = <-ackCh: // if ack returns first
		}

		if !ack.Success {
			return fmt.Errorf("%v", ack.Reason) // error occurs if ack not indicate sucess
		}
	}

	value := &topoapi.RSMSlicingItem{ // star instance RSMSlicingItem
		ID:        req.SliceId, // receives details about slice, ID receives slice ID from request
		SliceDesc: "Slice created by IQoS xAPP",
		SliceParameters: &topoapi.RSMSliceParameters{ // parameters are received by RSMSliceParameters
			SchedulerType: topoapi.RSMSchedulerType(req.SchedulerType),
			Weight:        weight,
		},
		SliceType: topoapi.RSMSliceType(req.SliceType), // slice type defined by request
		UeIdList:  make([]*topoapi.UeIdentity, 0),      // inicialize as empty list of id UEs, in other words, slice is created without UEs
	}

	err = m.rnibClient.AddRsmSliceItemAspect(ctx, topoapi.ID(req.E2NodeId), value) // if return an error, it wasn't possible to regiter slice in RNIB
	if err != nil {
		return fmt.Errorf("failed to create slice information to onos-topo although control message was sent: %v", err)
	}

	return nil
}

func (m *Manager) handleNbiDeleteSliceRequest(ctx context.Context, req *rsmapi.DeleteSliceRequest, nodeID topoapi.ID) error {
	log.Infof("Called Delete Slice: %v", req) //  log the function "delete slice" was called
	sliceID, err := strconv.Atoi(req.SliceId) //  convert slice id from string to integers
	if err != nil {
		return fmt.Errorf("failed to convert slice id to int - %v", err.Error()) // error case
	}

	cmdType := e2sm_rsm.E2SmRsmCommand_E2_SM_RSM_COMMAND_SLICE_DELETE

	var sliceType e2sm_rsm.SliceType // create a var of type "SliceType" defined in "e2sm_rsm"
	switch req.SliceType {           // start a swicth to define a Scheduler Type based on "SliceType" presents in "req"
	case rsmapi.SliceType_SLICE_TYPE_DL_SLICE:
		sliceType = e2sm_rsm.SliceType_SLICE_TYPE_DL_SLICE
	case rsmapi.SliceType_SLICE_TYPE_UL_SLICE:
		sliceType = e2sm_rsm.SliceType_SLICE_TYPE_UL_SLICE
	default:
		sliceType = e2sm_rsm.SliceType_SLICE_TYPE_DL_SLICE // default slice type if not a single type was indentified
	}

	sliceConfig := &e2sm_rsm.SliceConfig{ // create and start a instance of structure "SliceConfig" defined in e2sm_rsm
		SliceId: &e2sm_rsm.SliceId{ //instance of SliceId in e2sm_rsm
			Value: int64(sliceID), //included value of sliceID, converted into int64
		},
		SliceType: sliceType,
	}

	ctrlMsg, err := m.ctrlMsgHandler.CreateControlRequest(cmdType, sliceConfig, nil) //  create a control request E2 based on cmdType and Slice Config
	if err != nil {
		return fmt.Errorf("failed to create the control message - %v", err.Error()) // error case
	}

	hasSliceItem := m.rnibClient.HasRsmSliceItemAspect(ctx, topoapi.ID(req.E2NodeId), req.SliceId, req.SliceType)
	// verify if exist a slice with specified ID and slice type in RNIB through RNIB client. topoapi.ID(req.E2NodeId) indicate ID of E2 node where verication is carried out.
	if !hasSliceItem {
		return fmt.Errorf("no slice ID %v in node %v", sliceID, nodeID) // error case, if slice already exists.
	}

	// send control message
	ackCh := make(chan e2.Ack) // create a ack channel defined in e2. These channels are use to communication between go routines
	msg := &e2.CtrlMsg{        // create a instance CtrlMsg, represents a control msgs to be send
		CtrlMsg: ctrlMsg, // it has the  control msg and ack channel
		AckCh:   ackCh,
	}
	go func() {
		m.ctrlReqChsSliceDelete[string(nodeID)] <- msg // go routine to send control msg created in "msg" to specified channel do NodeID
	}()

	// ackTimer -1 is for uenib/topo debugging and integration test
	if m.ackTimer != -1 {
		var ack e2.Ack
		select { // wait events from multiple channels
		case <-time.After(time.Duration(m.ackTimer) * time.Second): // if timeout happens first, an error returns
			return fmt.Errorf("timeout happens: E2 SBI could not send ACK until timer expired")
		case ack = <-ackCh: // if ack returns first
		}
		if !ack.Success {
			return fmt.Errorf("%v", ack.Reason)
		}
	}

	err = m.rnibClient.DeleteRsmSliceItemAspect(ctx, nodeID, req.SliceId) // if return an error, it wasn't possible to delete slice in RNIB
	if err != nil {
		return fmt.Errorf("failed to delete slice information to onos-topo although control message was sent: %v", err)
	}

	ues, err := m.uenibClient.GetUEs(ctx) // try get a list of UEs registered in UENIB
	if err != nil {
		return fmt.Errorf("failed to get UEs in UENIB: %v", err) // error getting
	}

	for i := 0; i < len(ues); i++ { // start a loop over all the UEs obtained
		changed := false                             // indicate if the list of slices was changed
		for j := 0; j < len(ues[i].SliceList); j++ { // indicate second loop over list of slices associated with UEs
			if ues[i].SliceList[j].ID == req.SliceId && ues[i].SliceList[j].SliceType == uenib_api.RSMSliceType(req.SliceType) { // this condition verify if the id and slice type from  UE's slice list
				ues[i].SliceList = append(ues[i].SliceList[:j], ues[i].SliceList[j+1:]...) // match with requested id and slice type to delete
				j--                                                                        // after detect the slice to delete, uses the append function to concatenate two parts of the list, excluding the slice that matches the criterion
				changed = true                                                             // and turn true "changed"
			}
		}
		if changed {
			err = m.uenibClient.UpdateUE(ctx, ues[i]) // return a error if wasn't possible update UE's data
			if err != nil {
				return fmt.Errorf("failed to update UENIB: %v", err)
			}
		}
	}

	return nil
}

func (m *Manager) handleNbiUpdateSliceRequest(ctx context.Context, req *rsmapi.UpdateSliceRequest, nodeID topoapi.ID) error {
	log.Infof("Called Update Slice: %v", req) //  log the function "update slice" was called
	sliceID, err := strconv.Atoi(req.SliceId) //  convert slice id from string to integers
	if err != nil {
		return fmt.Errorf("failed to convert slice id to int - %v", err.Error()) // error case
	}
	weightInt, err := strconv.Atoi(req.Weight) // convert weight from string to integers
	if err != nil {
		return fmt.Errorf("failed to convert weight to int - %v", err.Error()) // error case
	}
	weight := int32(weightInt) // convert int to int32

	cmdType := e2sm_rsm.E2SmRsmCommand_E2_SM_RSM_COMMAND_SLICE_CREATE // cmdType's value represents a command to update slice
	var sliceSchedulerType e2sm_rsm.SchedulerType                     // create a var of type "SchedulerType" defined in "e2sm_rsm"
	switch req.SchedulerType {                                        // start a swicth to define a Scheduler Type based on "SchedulerType" presents in "req"
	case rsmapi.SchedulerType_SCHEDULER_TYPE_ROUND_ROBIN:
		sliceSchedulerType = e2sm_rsm.SchedulerType_SCHEDULER_TYPE_ROUND_ROBIN
	case rsmapi.SchedulerType_SCHEDULER_TYPE_PROPORTIONALLY_FAIR:
		sliceSchedulerType = e2sm_rsm.SchedulerType_SCHEDULER_TYPE_PROPORTIONALLY_FAIR
	case rsmapi.SchedulerType_SCHEDULER_TYPE_QOS_BASED:
		sliceSchedulerType = e2sm_rsm.SchedulerType_SCHEDULER_TYPE_QOS_BASED
	default:
		sliceSchedulerType = e2sm_rsm.SchedulerType_SCHEDULER_TYPE_ROUND_ROBIN // default scheduler type if not a single type was indentified
	}

	var sliceType e2sm_rsm.SliceType // create a var of type "SliceType" defined in "e2sm_rsm"
	switch req.SliceType {           // start a swicth to define a Scheduler Type based on "SliceType" presents in "req"
	case rsmapi.SliceType_SLICE_TYPE_DL_SLICE:
		sliceType = e2sm_rsm.SliceType_SLICE_TYPE_DL_SLICE
	case rsmapi.SliceType_SLICE_TYPE_UL_SLICE:
		sliceType = e2sm_rsm.SliceType_SLICE_TYPE_UL_SLICE
	default:
		sliceType = e2sm_rsm.SliceType_SLICE_TYPE_DL_SLICE // default slice type if not a single type was indentified
	}

	sliceConfig := &e2sm_rsm.SliceConfig{ // create and start a instance of structure "SliceConfig" defined in e2sm_rsm
		SliceId: &e2sm_rsm.SliceId{ //instance of SliceId in e2sm_rsm
			Value: int64(sliceID), //included value of sliceID, converted into int64
		},
		SliceConfigParameters: &e2sm_rsm.SliceParameters{ // instance of SliceParameters with SchedulerType and Weight
			SchedulerType: sliceSchedulerType,
			Weight:        &weight,
		},
		SliceType: sliceType, // define slice type
	}

	ctrlMsg, err := m.ctrlMsgHandler.CreateControlRequest(cmdType, sliceConfig, nil) //  create a control request E2 based on cmdType and Slice Config
	if err != nil {
		return fmt.Errorf("failed to create the control message - %v", err.Error()) // error case
	}

	hasSliceItem := m.rnibClient.HasRsmSliceItemAspect(ctx, topoapi.ID(req.E2NodeId), req.SliceId, req.GetSliceType())
	// verify if exist a slice with specified ID and slice type in RNIB through RNIB client. topoapi.ID (req.E2NodeId) indicate ID of E2 node where verication is carried out.
	if hasSliceItem {
		return fmt.Errorf("slice ID %v already exists", sliceID) // error case, if slice already exists.
	}

	// send control message
	ackCh := make(chan e2.Ack) // create a ack channel defined in e2. These channels are use to communication between go routines
	msg := &e2.CtrlMsg{        // create a instance CtrlMsg, represents a control msgs to be send
		CtrlMsg: ctrlMsg, // it has the  control msg and ack channel
		AckCh:   ackCh,
	}
	go func() {
		m.ctrlReqChsSliceUpdate[string(nodeID)] <- msg // go routine to send control msg created in "msg" to specified channel do NodeID
	}()

	// ackTimer -1 is for uenib/topo debugging and integration test
	if m.ackTimer != -1 {
		var ack e2.Ack
		select { // wait events from multiple channels
		case <-time.After(time.Duration(m.ackTimer) * time.Second): // if timeout happens first, an error returns
			return fmt.Errorf("timeout happens: E2 SBI could not send ACK until timer expired")
		case ack = <-ackCh: // if ack returns first
		}

		if !ack.Success {
			return fmt.Errorf("%v", ack.Reason) // error occurs if ack not indicate sucess
		}
	}

	sliceAspect, err := m.rnibClient.GetRsmSliceItemAspect(ctx, topoapi.ID(req.E2NodeId), req.SliceId, req.GetSliceType()) // try get slice aspect registered in RNIB client, with slice ID and E2 nodeID
	if err != nil {
		return fmt.Errorf("failed to get slice aspect - slice ID %v in node %v: err: %v", sliceID, nodeID, err) // error getting
	}

	ueIDList := sliceAspect.GetUeIdList() // verify which UEs are associeted with these slice and create a list
	if len(ueIDList) == 0 {               // verify if this list are empty,
		ueIDList = make([]*topoapi.UeIdentity, 0) // if was empty, the list is initialize as an empty list of pointers to topoapi.UeIdentity using the make function
	} // allow list to be ready to use, furthermore can grow dynamically as new elements are added

	value := &topoapi.RSMSlicingItem{ // star instance RSMSlicingItem
		ID:        req.SliceId, // receives details about slice, ID receives slice ID from request
		SliceDesc: "Slice created by IQoS xAPP",
		SliceParameters: &topoapi.RSMSliceParameters{ // parameters are received by RSMSliceParameters
			SchedulerType: topoapi.RSMSchedulerType(req.SchedulerType),
			Weight:        weight,
		},
		SliceType: topoapi.RSMSliceType(req.SliceType), // slice type defined by request
		UeIdList:  ueIDList,
	}

	err = m.rnibClient.UpdateRsmSliceItemAspect(ctx, topoapi.ID(req.E2NodeId), value) // if return an error, it wasn't possible to update slice aspect in RNIB
	if err != nil {
		return fmt.Errorf("failed to update slice information to onos-topo although control message was sent: %v", err)
	}

	ues, err := m.uenibClient.GetUEs(ctx) // try get a list of UEs registered in UENIB client
	if err != nil {
		return fmt.Errorf("failed to get UEs in UENIB: %v", err) // error getting
	}

	for i := 0; i < len(ues); i++ { // start a loop over all the UEs obtained
		changed := false                             // indicate if the list of slices was changed
		for j := 0; j < len(ues[i].SliceList); j++ { // indicate second loop over list of slices associated with UEs
			if ues[i].SliceList[j].ID == req.SliceId && ues[i].SliceList[j].SliceType == uenib_api.RSMSliceType(req.SliceType) { // this condition verify if the id and slice type from  UE's slice list
				ues[i].SliceList[j].SliceParameters.Weight = weight                                               // match with requested id and slice type to update
				ues[i].SliceList[j].SliceParameters.SchedulerType = uenib_api.RSMSchedulerType(req.SchedulerType) // update values about weight and/or scheduler type
				changed = true                                                                                    // and turn true "changed"
			}
		}
		if changed {
			err = m.uenibClient.UpdateUE(ctx, ues[i]) // return a error if wasn't possible update UE's data
			if err != nil {
				return fmt.Errorf("failed to update UENIB: %v", err)
			}
		}
	}

	return nil
}

func (m *Manager) handleNbiSetUeSliceAssociationRequest(ctx context.Context, req *rsmapi.SetUeSliceAssociationRequest, nodeID topoapi.ID) error {
	log.Infof("Called SetUeSliceAssociation: %v", req)
	var err error
	duNodeID := req.E2NodeId
	cuNodeID, err := m.rnibClient.GetSourceCUE2NodeID(ctx, topoapi.ID(duNodeID))
	if err != nil {
		return fmt.Errorf("DU %v does not have CU in onos-topo (RNIB) - please add or update CU-DU relation: %v", duNodeID, err)
	}

	var DuUeF1apID, CuUeF1apID, RanUeNgapID, AmfUeNgapID, EnbUeS1apID, drbID int64
	for _, tmpID := range req.GetUeId() {
		if tmpID.GetUeId() == "" {
			continue
		}
		switch tmpID.GetType() {
		case rsmapi.UeIdType_UE_ID_TYPE_CU_UE_F1_AP_ID:
			CuUeF1apID, err = strconv.ParseInt(tmpID.GetUeId(), 10, 64)
		case rsmapi.UeIdType_UE_ID_TYPE_DU_UE_F1_AP_ID:
			DuUeF1apID, err = strconv.ParseInt(tmpID.GetUeId(), 10, 64)
		case rsmapi.UeIdType_UE_ID_TYPE_RAN_UE_NGAP_ID:
			RanUeNgapID, err = strconv.ParseInt(tmpID.GetUeId(), 10, 64)
		case rsmapi.UeIdType_UE_ID_TYPE_AMF_UE_NGAP_ID:
			AmfUeNgapID, err = strconv.ParseInt(tmpID.GetUeId(), 10, 64)
		case rsmapi.UeIdType_UE_ID_TYPE_ENB_UE_S1_AP_ID:
			EnbUeS1apID, err = strconv.ParseInt(tmpID.GetUeId(), 10, 32)
		default:
			err = fmt.Errorf("invalid ID type %v", tmpID)
		}
		if err != nil {
			return fmt.Errorf("Invalid ID format %v - %v", tmpID, err)
		}
	}

	drbID, err = strconv.ParseInt(req.DrbId, 10, 32)
	if err != nil {
		return fmt.Errorf("failed to convert drb-id to int - %v", err)
	}

	cmdType := e2sm_rsm.E2SmRsmCommand_E2_SM_RSM_COMMAND_UE_ASSOCIATE

	hasDlSliceID := false
	dlSliceID := 0
	if req.DlSliceId != "" {
		dlSliceID, err = strconv.Atoi(req.DlSliceId)
		if err != nil {
			return fmt.Errorf("failed to convert slice id to int: %v", err)
		}
		hasDlSliceID = true
	}

	hasUlSliceID := false
	ulSliceID := 0
	if req.UlSliceId != "" {
		ulSliceID, err = strconv.Atoi(req.UlSliceId)
		if err != nil {
			return fmt.Errorf("failed to convert slice id to int - %v", err)
		}
		hasUlSliceID = true
	}

	if !hasDlSliceID && !hasUlSliceID {
		return fmt.Errorf("both DL slice ID and UL slice ID are empty: %v", *req)
	}

	var reqUeID int64
	hasValidUeID := false
	for _, ueid := range req.UeId {
		if ueid.GetType() == rsmapi.UeIdType_UE_ID_TYPE_DU_UE_F1_AP_ID {
			hasValidUeID = true
			id, err := strconv.Atoi(ueid.GetUeId())
			if err != nil {
				return fmt.Errorf("failed to convert ue id to int - %v", err)
			}
			reqUeID = int64(id)
		}
	}

	if !hasValidUeID {
		return fmt.Errorf("need valid du-ue-f1ap-id")
	}

	hasUlSliceItem := m.rnibClient.HasRsmSliceItemAspect(ctx, topoapi.ID(duNodeID), req.GetUlSliceId(), rsmapi.SliceType_SLICE_TYPE_UL_SLICE)
	hasDlSliceItem := m.rnibClient.HasRsmSliceItemAspect(ctx, topoapi.ID(duNodeID), req.GetDlSliceId(), rsmapi.SliceType_SLICE_TYPE_DL_SLICE)

	if !hasUlSliceItem && !hasDlSliceItem {
		return fmt.Errorf("invalid slice ID")
	}

	ueID := &e2sm_rsm.UeIdentity{
		UeIdentity: &e2sm_rsm.UeIdentity_DuUeF1ApId{
			DuUeF1ApId: &e2sm_rsm.DuUeF1ApId{
				Value: reqUeID,
			},
		},
	}

	rsmUEInfo, err := m.uenibClient.GetUEWithPreferredID(ctx, string(cuNodeID), uenib_api.UeIdType_UE_ID_TYPE_DU_UE_F1_AP_ID, DuUeF1apID)
	if err != nil {
		return fmt.Errorf("failed to get UENIB UE info (CuID %v DUID %v UEID %v): err: %v", cuNodeID, duNodeID, ueID, err)
	}

	if rsmUEInfo.GetDuE2NodeId() == "" {
		rsmUEInfo.DuE2NodeId = duNodeID
	} else if rsmUEInfo.GetDuE2NodeId() != duNodeID {
		return fmt.Errorf("DU ID in UENIB and received DU ID are not matched - received DU ID: %v DU ID in uenib: %v", duNodeID, rsmUEInfo.GetDuE2NodeId())
	}

	bearerIDs := make([]*e2sm_rsm.BearerId, 0)
	var bearerID *e2sm_rsm.BearerId
	for _, bID := range rsmUEInfo.GetBearerIdList() {
		if bID.GetDrbId().GetFiveGdrbId() != nil && bID.GetDrbId().GetFiveGdrbId().GetValue() == int32(drbID) {
			bearerID = &e2sm_rsm.BearerId{
				BearerId: &e2sm_rsm.BearerId_DrbId{
					DrbId: &e2sm_rsm.DrbId{
						DrbId: &e2sm_rsm.DrbId_FiveGdrbId{
							FiveGdrbId: &e2sm_rsm.FiveGDrbId{
								Value: bID.GetDrbId().GetFiveGdrbId().GetValue(),
							},
						},
					},
				},
			}
			if bID.GetDrbId().GetFiveGdrbId().GetQfi() != nil {
				bearerID.GetDrbId().GetFiveGdrbId().Qfi = &e2sm_rsm.Qfi{
					Value: bID.GetDrbId().GetFiveGdrbId().GetQfi().Value,
				}
			}
			if len(bID.GetDrbId().GetFiveGdrbId().GetFlowsMapToDrb()) != 0 {
				bearerID.GetDrbId().GetFiveGdrbId().FlowsMapToDrb = make([]*e2sm_rsm.QoSflowLevelParameters, 0)
				for _, flow := range bID.GetDrbId().GetFiveGdrbId().GetFlowsMapToDrb() {
					var param *e2sm_rsm.QoSflowLevelParameters
					if flow.GetNonDynamicFiveQi() != nil {
						param = &e2sm_rsm.QoSflowLevelParameters{
							QoSflowLevelParameters: &e2sm_rsm.QoSflowLevelParameters_NonDynamicFiveQi{
								NonDynamicFiveQi: &e2sm_rsm.NonDynamicFiveQi{
									FiveQi: &e2sm_v2_ies.FiveQi{
										Value: flow.GetNonDynamicFiveQi().GetFiveQi().GetValue(),
									},
								},
							},
						}
					}
					if flow.GetDynamicFiveQi() != nil {
						param = &e2sm_rsm.QoSflowLevelParameters{
							QoSflowLevelParameters: &e2sm_rsm.QoSflowLevelParameters_DynamicFiveQi{
								DynamicFiveQi: &e2sm_rsm.DynamicFiveQi{
									PriorityLevel:     flow.GetDynamicFiveQi().GetPriorityLevel(),
									PacketDelayBudget: flow.GetDynamicFiveQi().GetPacketDelayBudge(),
									PacketErrorRate:   flow.GetDynamicFiveQi().GetPacketErrorRate(),
								},
							},
						}
					}
					bearerID.GetDrbId().GetFiveGdrbId().FlowsMapToDrb = append(bearerID.GetDrbId().GetFiveGdrbId().FlowsMapToDrb, param)
				}
			}
			bearerIDs = append(bearerIDs, bearerID)
		}
		if bID.GetDrbId().GetFourGdrbId() != nil && bID.GetDrbId().GetFourGdrbId().GetValue() == int32(drbID) {
			bearerID = &e2sm_rsm.BearerId{
				BearerId: &e2sm_rsm.BearerId_DrbId{
					DrbId: &e2sm_rsm.DrbId{
						DrbId: &e2sm_rsm.DrbId_FourGdrbId{
							FourGdrbId: &e2sm_rsm.FourGDrbId{
								Value: bID.GetDrbId().GetFourGdrbId().GetValue(),
							},
						},
					},
				},
			}
			if bID.GetDrbId().GetFourGdrbId().GetQci() != nil {
				bearerID.GetDrbId().GetFourGdrbId().Qci = &e2sm_v2_ies.Qci{
					Value: bID.GetDrbId().GetFourGdrbId().GetQci().GetValue(),
				}
			}
			bearerIDs = append(bearerIDs, bearerID)
		}
	}

	if len(bearerIDs) == 0 {
		return fmt.Errorf("the number of bearers is 0")
	}

	sliceAssoc := &e2sm_rsm.SliceAssociate{
		DownLinkSliceId: &e2sm_rsm.SliceIdassoc{
			Value: int64(dlSliceID),
		},
		UeId:     ueID,
		BearerId: bearerIDs,
	}
	if hasUlSliceID {
		sliceAssoc.UplinkSliceId = &e2sm_rsm.SliceIdassoc{
			Value: int64(ulSliceID),
		}
	}

	ctrlMsg, err := m.ctrlMsgHandler.CreateControlRequest(cmdType, nil, sliceAssoc)
	if err != nil {
		return fmt.Errorf("failed to create the control message - %v", err)
	}

	// send control message
	ackCh := make(chan e2.Ack)
	msg := &e2.CtrlMsg{
		CtrlMsg: ctrlMsg,
		AckCh:   ackCh,
	}
	go func() {
		m.ctrlReqChsUeAssociate[string(nodeID)] <- msg
	}()

	// ackTimer -1 is for uenib/topo debugging and integration test
	if m.ackTimer != -1 {
		var ack e2.Ack
		select {
		case <-time.After(time.Duration(m.ackTimer) * time.Second):
			return fmt.Errorf("timeout happens: E2 SBI could not send ACK until timer expired")
		case ack = <-ackCh:
		}

		if !ack.Success {
			return fmt.Errorf("%v", ack.Reason)
		}
	}

	err = m.uenibClient.UpdateUE(ctx, rsmUEInfo)
	if err != nil {
		return fmt.Errorf("tried to update du e2node ID on uenib (because there was no du ID) but failed to update du id UENIB UE info (CuID %v DUID %v UEID %v uenib UE info %v): err: %v", cuNodeID, duNodeID, ueID, rsmUEInfo, err)
	}

	// Update topo
	ueIDforTopo := &topoapi.UeIdentity{
		DuUeF1apID: &topoapi.DuUeF1ApID{
			Value: DuUeF1apID,
		},
		CuUeF1apID: &topoapi.CuUeF1ApID{
			Value: CuUeF1apID,
		},
		RANUeNgapID: &topoapi.RanUeNgapID{
			Value: RanUeNgapID,
		},
		AMFUeNgapID: &topoapi.AmfUeNgapID{
			Value: AmfUeNgapID,
		},
		EnbUeS1apID: &topoapi.EnbUeS1ApID{
			Value: int32(EnbUeS1apID),
		},
	}

	var topoDrbID *topoapi.DrbId
	var uenibDrbID *uenib_api.DrbId
	for _, bID := range rsmUEInfo.GetBearerIdList() {
		if bID.GetDrbId().GetFiveGdrbId() != nil && bID.GetDrbId().GetFiveGdrbId().GetValue() == int32(drbID) {
			topoDrbID = &topoapi.DrbId{
				DrbId: &topoapi.DrbId_FiveGdrbId{
					FiveGdrbId: &topoapi.FiveGDrbId{
						Value: bID.GetDrbId().GetFiveGdrbId().GetValue(),
					},
				},
			}
			uenibDrbID = &uenib_api.DrbId{
				DrbId: &uenib_api.DrbId_FiveGdrbId{
					FiveGdrbId: &uenib_api.FiveGDrbId{
						Value: bID.GetDrbId().GetFiveGdrbId().GetValue(),
					},
				},
			}
			if bID.GetDrbId().GetFiveGdrbId().GetQfi() != nil {
				topoDrbID.GetFiveGdrbId().Qfi = &topoapi.Qfi{
					Value: bID.GetDrbId().GetFiveGdrbId().GetQfi().GetValue(),
				}
				uenibDrbID.GetFiveGdrbId().Qfi = &uenib_api.Qfi{
					Value: bID.GetDrbId().GetFiveGdrbId().GetQfi().GetValue(),
				}
			}
			if len(bID.GetDrbId().GetFiveGdrbId().GetFlowsMapToDrb()) != 0 {
				topoDrbID.GetFiveGdrbId().FlowsMapToDrb = make([]*topoapi.QoSflowLevelParameters, 0)
				uenibDrbID.GetFiveGdrbId().FlowsMapToDrb = make([]*uenib_api.QoSflowLevelParameters, 0)
				for _, flow := range bID.GetDrbId().GetFiveGdrbId().GetFlowsMapToDrb() {
					var paramTopo *topoapi.QoSflowLevelParameters
					var paramUenib *uenib_api.QoSflowLevelParameters
					if flow.GetNonDynamicFiveQi() != nil {
						paramTopo = &topoapi.QoSflowLevelParameters{
							QosFlowLevelParameters: &topoapi.QoSflowLevelParameters_NonDynamicFiveQi{
								NonDynamicFiveQi: &topoapi.NonDynamicFiveQi{
									FiveQi: &topoapi.FiveQi{
										Value: flow.GetNonDynamicFiveQi().GetFiveQi().GetValue(),
									},
								},
							},
						}
						paramUenib = &uenib_api.QoSflowLevelParameters{
							QosFlowLevelParameters: &uenib_api.QoSflowLevelParameters_NonDynamicFiveQi{
								NonDynamicFiveQi: &uenib_api.NonDynamicFiveQi{
									FiveQi: &uenib_api.FiveQi{
										Value: flow.GetNonDynamicFiveQi().GetFiveQi().GetValue(),
									},
								},
							},
						}
					}
					if flow.GetDynamicFiveQi() != nil {
						paramTopo = &topoapi.QoSflowLevelParameters{
							QosFlowLevelParameters: &topoapi.QoSflowLevelParameters_DynamicFiveQi{
								DynamicFiveQi: &topoapi.DynamicFiveQi{
									PriorityLevel:    flow.GetDynamicFiveQi().GetPriorityLevel(),
									PacketDelayBudge: flow.GetDynamicFiveQi().GetPacketDelayBudge(),
									PacketErrorRate:  flow.GetDynamicFiveQi().GetPacketErrorRate(),
								},
							},
						}
						paramUenib = &uenib_api.QoSflowLevelParameters{
							QosFlowLevelParameters: &uenib_api.QoSflowLevelParameters_DynamicFiveQi{
								DynamicFiveQi: &uenib_api.DynamicFiveQi{
									PriorityLevel:    flow.GetDynamicFiveQi().GetPriorityLevel(),
									PacketDelayBudge: flow.GetDynamicFiveQi().GetPacketDelayBudge(),
									PacketErrorRate:  flow.GetDynamicFiveQi().GetPacketErrorRate(),
								},
							},
						}
					}
					topoDrbID.GetFiveGdrbId().FlowsMapToDrb = append(topoDrbID.GetFiveGdrbId().FlowsMapToDrb, paramTopo)
					uenibDrbID.GetFiveGdrbId().FlowsMapToDrb = append(uenibDrbID.GetFiveGdrbId().FlowsMapToDrb, paramUenib)
				}
			}
		}
		if bID.GetDrbId().GetFourGdrbId() != nil && bID.GetDrbId().GetFourGdrbId().GetValue() == int32(drbID) {
			topoDrbID = &topoapi.DrbId{
				DrbId: &topoapi.DrbId_FourGdrbId{
					FourGdrbId: &topoapi.FourGDrbId{
						Value: bID.GetDrbId().GetFourGdrbId().GetValue(),
					},
				},
			}
			uenibDrbID = &uenib_api.DrbId{
				DrbId: &uenib_api.DrbId_FourGdrbId{
					FourGdrbId: &uenib_api.FourGDrbId{
						Value: bID.GetDrbId().GetFourGdrbId().GetValue(),
					},
				},
			}
			if bID.GetDrbId().GetFourGdrbId().GetQci() != nil {
				topoDrbID.GetFourGdrbId().Qci = &topoapi.Qci{
					Value: bID.GetDrbId().GetFourGdrbId().GetQci().GetValue(),
				}
				uenibDrbID.GetFourGdrbId().Qci = &uenib_api.Qci{
					Value: bID.GetDrbId().GetFourGdrbId().GetQci().GetValue(),
				}
			}
		}
	}

	if hasUlSliceItem {
		ulSliceItems, err := m.rnibClient.GetRsmSliceItemAspects(ctx, topoapi.ID(duNodeID))
		if err != nil {
			return fmt.Errorf("failed to get slice item list from R-NIB: %v", err)
		}
		for _, oldDlItem := range ulSliceItems {
			changed := false
			for i := 0; i < len(oldDlItem.UeIdList); i++ {
				if oldDlItem.UeIdList[i].GetDrbId().GetFiveGdrbId() != nil {
					if oldDlItem.UeIdList[i].GetDrbId().GetFiveGdrbId().GetValue() == int32(drbID) &&
						oldDlItem.UeIdList[i].GetDuUeF1apID().GetValue() == DuUeF1apID &&
						oldDlItem.SliceType == topoapi.RSMSliceType_SLICE_TYPE_UL_SLICE {
						oldDlItem.UeIdList = append(oldDlItem.UeIdList[:i], oldDlItem.UeIdList[i+1:]...)
						i--
						changed = true
					}
				} else if oldDlItem.UeIdList[i].GetDrbId().GetFourGdrbId() != nil && oldDlItem.UeIdList[i].GetDuUeF1apID().GetValue() == DuUeF1apID {
					if oldDlItem.UeIdList[i].GetDrbId().GetFourGdrbId().GetValue() == int32(drbID) &&
						oldDlItem.UeIdList[i].GetDuUeF1apID().GetValue() == DuUeF1apID &&
						oldDlItem.SliceType == topoapi.RSMSliceType_SLICE_TYPE_UL_SLICE {
						oldDlItem.UeIdList = append(oldDlItem.UeIdList[:i], oldDlItem.UeIdList[i+1:]...)
						i--
						changed = true
					}
				}
			}
			if changed {
				err = m.rnibClient.UpdateRsmSliceItemAspect(ctx, topoapi.ID(req.GetE2NodeId()), oldDlItem)
				if err != nil {
					return fmt.Errorf("failed to update UL slice item top onos-topo(ID - %v, sliceID - %v, sliceType - %v): %v", duNodeID, req.GetUlSliceId(), rsmapi.SliceType_SLICE_TYPE_UL_SLICE, err)
				}
			}
		}

		ulSliceItem, err := m.rnibClient.GetRsmSliceItemAspect(ctx, topoapi.ID(duNodeID), req.GetUlSliceId(), rsmapi.SliceType_SLICE_TYPE_UL_SLICE)
		if err != nil {
			return fmt.Errorf("failed to get UL slice item (ID - %v, sliceID - %v, sliceType - %v): %v", duNodeID, req.GetUlSliceId(), rsmapi.SliceType_SLICE_TYPE_UL_SLICE, err)
		}

		if len(ulSliceItem.GetUeIdList()) == 0 {
			ulSliceItem.UeIdList = make([]*topoapi.UeIdentity, 0)
		}

		ueIDforTopo.DrbId = topoDrbID
		ulSliceItem.UeIdList = append(ulSliceItem.UeIdList, ueIDforTopo)

		err = m.rnibClient.UpdateRsmSliceItemAspect(ctx, topoapi.ID(req.GetE2NodeId()), ulSliceItem)
		if err != nil {
			return fmt.Errorf("failed to update UL slice item top onos-topo(ID - %v, sliceID - %v, sliceType - %v): %v", duNodeID, req.GetUlSliceId(), rsmapi.SliceType_SLICE_TYPE_UL_SLICE, err)
		}

		// Update uenib
		if rsmUEInfo.GetSliceList() == nil || len(rsmUEInfo.GetSliceList()) == 0 {
			rsmUEInfo.SliceList = make([]*uenib_api.SliceInfo, 0)
		}

		var ulSliceSchedulerType uenib_api.RSMSchedulerType
		switch ulSliceItem.GetSliceParameters().GetSchedulerType() {
		case topoapi.RSMSchedulerType_SCHEDULER_TYPE_ROUND_ROBIN:
			ulSliceSchedulerType = uenib_api.RSMSchedulerType_SCHEDULER_TYPE_ROUND_ROBIN
		case topoapi.RSMSchedulerType_SCHEDULER_TYPE_PROPORTIONALLY_FAIR:
			ulSliceSchedulerType = uenib_api.RSMSchedulerType_SCHEDULER_TYPE_PROPORTIONALLY_FAIR
		case topoapi.RSMSchedulerType_SCHEDULER_TYPE_QOS_BASED:
			ulSliceSchedulerType = uenib_api.RSMSchedulerType_SCHEDULER_TYPE_QOS_BASED
		default:
			return fmt.Errorf("not supported scheduler type: %v", ulSliceItem.GetSliceParameters().GetSchedulerType())
		}

		isUenibSliceUpdated := false
		for i := 0; i < len(rsmUEInfo.SliceList); i++ {
			if rsmUEInfo.SliceList[i].GetDrbId().GetFiveGdrbId() != nil {
				if rsmUEInfo.SliceList[i].GetDrbId().GetFiveGdrbId().GetValue() == int32(drbID) {
					rsmUEInfo.SliceList[i].DrbId = uenibDrbID
					isUenibSliceUpdated = true
				}
			}
			if rsmUEInfo.SliceList[i].GetDrbId().GetFourGdrbId() != nil {
				if rsmUEInfo.SliceList[i].GetDrbId().GetFourGdrbId().GetValue() == int32(drbID) {
					rsmUEInfo.SliceList[i].DrbId = uenibDrbID
					isUenibSliceUpdated = true
				}
			}
		}

		if !isUenibSliceUpdated {
			sliceInfo := &uenib_api.SliceInfo{
				DuE2NodeId: duNodeID,
				CuE2NodeId: string(cuNodeID),
				ID:         req.GetUlSliceId(),
				SliceParameters: &uenib_api.RSMSliceParameters{
					SchedulerType: ulSliceSchedulerType,
					Weight:        ulSliceItem.GetSliceParameters().Weight,
					QosLevel:      ulSliceItem.GetSliceParameters().QosLevel,
				},
				SliceType: uenib_api.RSMSliceType_SLICE_TYPE_UL_SLICE,
				DrbId:     uenibDrbID,
			}

			rsmUEInfo.SliceList = append(rsmUEInfo.SliceList, sliceInfo)
		}
		err = m.uenibClient.UpdateUE(ctx, rsmUEInfo)
		if err != nil {
			return fmt.Errorf("Failed to update uenib: %v", err)
		}
	}

	if hasDlSliceItem {
		dlSliceItems, err := m.rnibClient.GetRsmSliceItemAspects(ctx, topoapi.ID(duNodeID))
		if err != nil {
			return fmt.Errorf("failed to get slice item list from R-NIB: %v", err)
		}
		for _, oldDlItem := range dlSliceItems {
			changed := false
			for i := 0; i < len(oldDlItem.UeIdList); i++ {
				if oldDlItem.UeIdList[i].GetDrbId().GetFiveGdrbId() != nil {
					if oldDlItem.UeIdList[i].GetDrbId().GetFiveGdrbId().GetValue() == int32(drbID) &&
						oldDlItem.UeIdList[i].GetDuUeF1apID().GetValue() == DuUeF1apID &&
						oldDlItem.SliceType == topoapi.RSMSliceType_SLICE_TYPE_DL_SLICE {
						oldDlItem.UeIdList = append(oldDlItem.UeIdList[:i], oldDlItem.UeIdList[i+1:]...)
						i--
						changed = true
					}
				} else if oldDlItem.UeIdList[i].GetDrbId().GetFourGdrbId() != nil && oldDlItem.UeIdList[i].GetDuUeF1apID().GetValue() == DuUeF1apID {
					if oldDlItem.UeIdList[i].GetDrbId().GetFourGdrbId().GetValue() == int32(drbID) &&
						oldDlItem.UeIdList[i].GetDuUeF1apID().GetValue() == DuUeF1apID &&
						oldDlItem.SliceType == topoapi.RSMSliceType_SLICE_TYPE_DL_SLICE {
						oldDlItem.UeIdList = append(oldDlItem.UeIdList[:i], oldDlItem.UeIdList[i+1:]...)
						i--
						changed = true
					}
				}
			}
			if changed {
				err = m.rnibClient.UpdateRsmSliceItemAspect(ctx, topoapi.ID(req.GetE2NodeId()), oldDlItem)
				if err != nil {
					return fmt.Errorf("failed to update UL slice item top onos-topo(ID - %v, sliceID - %v, sliceType - %v): %v", duNodeID, req.GetUlSliceId(), rsmapi.SliceType_SLICE_TYPE_UL_SLICE, err)
				}
			}
		}

		dlSliceItem, err := m.rnibClient.GetRsmSliceItemAspect(ctx, topoapi.ID(duNodeID), req.GetDlSliceId(), rsmapi.SliceType_SLICE_TYPE_DL_SLICE)
		if err != nil {
			return fmt.Errorf("failed to get DL slice item (ID - %v, sliceID - %v, sliceType - %v): %v", duNodeID, req.GetDlSliceId(), rsmapi.SliceType_SLICE_TYPE_DL_SLICE, err)
		}

		if len(dlSliceItem.GetUeIdList()) == 0 {
			dlSliceItem.UeIdList = make([]*topoapi.UeIdentity, 0)
		}

		ueIDforTopo.DrbId = topoDrbID
		dlSliceItem.UeIdList = append(dlSliceItem.UeIdList, ueIDforTopo)

		err = m.rnibClient.UpdateRsmSliceItemAspect(ctx, topoapi.ID(req.GetE2NodeId()), dlSliceItem)
		if err != nil {
			return fmt.Errorf("failed to update UL slice item top onos-topo(ID - %v, sliceID - %v, sliceType - %v): %v", duNodeID, req.GetUlSliceId(), rsmapi.SliceType_SLICE_TYPE_UL_SLICE, err)
		}

		// Update uenib
		if rsmUEInfo.GetSliceList() == nil || len(rsmUEInfo.GetSliceList()) == 0 {
			rsmUEInfo.SliceList = make([]*uenib_api.SliceInfo, 0)
		}

		var dlSliceSchedulerType uenib_api.RSMSchedulerType
		switch dlSliceItem.GetSliceParameters().GetSchedulerType() {
		case topoapi.RSMSchedulerType_SCHEDULER_TYPE_ROUND_ROBIN:
			dlSliceSchedulerType = uenib_api.RSMSchedulerType_SCHEDULER_TYPE_ROUND_ROBIN
		case topoapi.RSMSchedulerType_SCHEDULER_TYPE_PROPORTIONALLY_FAIR:
			dlSliceSchedulerType = uenib_api.RSMSchedulerType_SCHEDULER_TYPE_PROPORTIONALLY_FAIR
		case topoapi.RSMSchedulerType_SCHEDULER_TYPE_QOS_BASED:
			dlSliceSchedulerType = uenib_api.RSMSchedulerType_SCHEDULER_TYPE_QOS_BASED
		default:
			return fmt.Errorf("not supported scheduler type: %v", dlSliceItem.GetSliceParameters().GetSchedulerType())
		}

		for i := 0; i < len(rsmUEInfo.SliceList); i++ {
			if rsmUEInfo.SliceList[i].GetDrbId().GetFiveGdrbId() != nil {
				if rsmUEInfo.SliceList[i].GetDrbId().GetFiveGdrbId().GetValue() == int32(drbID) {
					rsmUEInfo.SliceList = append(rsmUEInfo.SliceList[:i], rsmUEInfo.SliceList[i+1:]...)
					i--
				}
			}
			if rsmUEInfo.SliceList[i].GetDrbId().GetFourGdrbId() != nil {
				if rsmUEInfo.SliceList[i].GetDrbId().GetFourGdrbId().GetValue() == int32(drbID) {
					rsmUEInfo.SliceList = append(rsmUEInfo.SliceList[:i], rsmUEInfo.SliceList[i+1:]...)
					i--
				}
			}
		}

		sliceInfo := &uenib_api.SliceInfo{
			DuE2NodeId: duNodeID,
			CuE2NodeId: string(cuNodeID),
			ID:         req.GetDlSliceId(),
			SliceParameters: &uenib_api.RSMSliceParameters{
				SchedulerType: dlSliceSchedulerType,
				Weight:        dlSliceItem.GetSliceParameters().Weight,
				QosLevel:      dlSliceItem.GetSliceParameters().QosLevel,
			},
			SliceType: uenib_api.RSMSliceType_SLICE_TYPE_DL_SLICE,
			DrbId:     uenibDrbID,
		}

		rsmUEInfo.SliceList = append(rsmUEInfo.SliceList, sliceInfo)

		err = m.uenibClient.UpdateUE(ctx, rsmUEInfo)
		if err != nil {
			return fmt.Errorf("Failed to update uenib: %v", err)
		}
	}

	return nil
}
