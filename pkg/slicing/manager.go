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
		//ctrlReqChsUeAssociate: options.Chans.CtrlReqChsUeAssociate,//control request channels to associate UEs in a specific slice
		rnibClient:     options.App.RnibClient, // clients RNIB and UENIB
		uenibClient:    options.App.UenibClient,
		ctrlMsgHandler: e2.NewControlMessageHandler(), // manipulator of control mensages
		ackTimer:       options.App.AckTimer,          // timer
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
