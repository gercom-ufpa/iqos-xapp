package uenib

import (
	"context"
	"fmt"
	"io"
	"strconv"

	uenib_api "github.com/onosproject/onos-api/go/onos/uenib"
	"github.com/onosproject/onos-lib-go/pkg/errors"
	"github.com/onosproject/onos-lib-go/pkg/logging"
	"github.com/onosproject/onos-lib-go/pkg/southbound"
)

var log = logging.GetLogger("iqos-xapp", "uenib")

// creates a new UE Client
func NewClient(ctx context.Context, config Config) (Client, error) {
	log.Info("Creating a new UE-NIB client")
	// onos-uenib address
	ueNibServiceAddress := config.UeNibEndpoint + ":" + strconv.Itoa(config.UeNibPort)
	// connects to UE NIB service
	conn, err := southbound.Connect(ctx, ueNibServiceAddress, config.CertPath, config.KeyPath)
	if err != nil {
		return Client{}, err
	}
	// creates a ue-nib client
	uenibClient := uenib_api.NewUEServiceClient(conn)

	return Client{
		uenibClient: uenibClient,
		config:      config,
	}, nil
}

// <--- To Slice Manager module ---->

// Gets UE through your global ID | return an UE
func (c *Client) getUEWithGlobalID(ctx context.Context, ueGlobalId string) (uenib_api.UE, error) {
	// creates an UE request
	req := &uenib_api.GetUERequest{
		ID: uenib_api.ID(ueGlobalId),
	}

	// gets an UE response
	resp, err := c.uenibClient.GetUE(ctx, req)
	if err != nil {
		return uenib_api.UE{}, err
	}

	// return an UE
	return resp.GetUE(), nil
}

// Gets UEs on the network | return RsmUeInfo aspect of each UE
func (c *Client) GetRsmUEs(ctx context.Context) ([]*uenib_api.RsmUeInfo, error) {
	// creates a result with RsmUeInfo aspect format
	result := make([]*uenib_api.RsmUeInfo, 0)

	// creates a list UEs stream
	stream, err := c.uenibClient.ListUEs(ctx, &uenib_api.ListUERequest{})
	if err != nil {
		return []*uenib_api.RsmUeInfo{}, err
	}

	// iterate over the stream
	for {
		// get a stream item
		response, err := stream.Recv()
		if err == io.EOF {
			break
		} else if err != nil {
			return []*uenib_api.RsmUeInfo{}, err
		}
		// gets UE
		ue := response.GetUE()
		// RsmUeInfo aspect
		rsmUE := &uenib_api.RsmUeInfo{}
		err = ue.GetAspect(rsmUE)
		if err != nil {
			return []*uenib_api.RsmUeInfo{}, err
		}

		result = append(result, rsmUE)
	}
	return result, nil
}

// Checks if a RSM UE already exists
func (c *Client) HasRsmUE(ctx context.Context, rmsUE *uenib_api.RsmUeInfo) bool {
	// gets a list of UEs with RSM aspects
	rsmUes, err := c.GetRsmUEs(ctx)
	if err != nil {
		log.Debug("onos-uenib has no UE")
		return false
	}

	// iterate of UEs
	for _, item := range rsmUes {
		// TODO: Is it necessary to check all these values?
		if item.GetGlobalUeID() == rmsUE.GetGlobalUeID() &&
			item.GetUeIdList().GetDuUeF1apID().Value == rmsUE.GetUeIdList().GetDuUeF1apID().Value &&
			item.GetUeIdList().GetCuUeF1apID().Value == rmsUE.GetUeIdList().GetCuUeF1apID().Value &&
			item.GetUeIdList().GetRANUeNgapID().Value == rmsUE.GetUeIdList().GetRANUeNgapID().Value && // for 5G ?
			item.GetUeIdList().GetEnbUeS1apID().Value == rmsUE.GetUeIdList().GetEnbUeS1apID().Value && // for 4G
			item.GetUeIdList().GetAMFUeNgapID().Value == rmsUE.GetUeIdList().GetAMFUeNgapID().Value && // for 5G
			item.GetUeIdList().GetPreferredIDType().String() == rmsUE.GetUeIdList().GetPreferredIDType().String() &&
			item.GetCellGlobalId() == rmsUE.GetCellGlobalId() &&
			item.GetCuE2NodeId() == rmsUE.GetCuE2NodeId() && item.GetDuE2NodeId() == rmsUE.GetDuE2NodeId() {
			return true
		}
	}

	log.Debugf("onos-uenib has UE %v", *rmsUE)
	return false
}

// Adds an UE with RsmUEInfo aspect
func (c *Client) AddRsmUE(ctx context.Context, rmsUE *uenib_api.RsmUeInfo) error {
	log.Debugf("received ue: %v", rmsUE)

	// checks if UE already exist
	if c.HasRsmUE(ctx, rmsUE) {
		return errors.NewAlreadyExists(fmt.Sprintf("UE already exists - UE: %v", *rmsUE))
	}

	// creates an obj UE
	uenibObj := uenib_api.UE{
		ID: uenib_api.ID(rmsUE.GetGlobalUeID()),
	}

	// sets aspect
	uenibObj.SetAspect(rmsUE)

	// creates an UE create request
	req := &uenib_api.CreateUERequest{
		UE: uenibObj,
	}

	// creates UE
	resp, err := c.uenibClient.CreateUE(ctx, req)
	if err != nil {
		log.Warn(err)
	}

	log.Debugf("AddRsmUE Resp: %v", resp)
	return nil
}

// Updates an existing RSM UE
func (c *Client) UpdateRsmUE(ctx context.Context, rsmUE *uenib_api.RsmUeInfo) error {
	// checks if UE already exist
	if !c.HasRsmUE(ctx, rsmUE) {
		return errors.NewNotFound(fmt.Sprintf("UE not found - UE: %v", *rsmUE))
	}

	// creates an obj UE
	uenibObj := uenib_api.UE{
		ID: uenib_api.ID(rsmUE.GetGlobalUeID()),
	}

	// sets aspect
	uenibObj.SetAspect(rsmUE)

	// creates an UE create request
	req := &uenib_api.UpdateUERequest{
		UE: uenibObj,
	}

	// updates UE
	resp, err := c.uenibClient.UpdateUE(ctx, req)
	if err != nil {
		return err
	}

	log.Debugf("UpdateUE Resp: %v", resp)

	return nil
}

// Deletes an existing UE
func (c *Client) DeleteUE(ctx context.Context, ueGlobalId string) error {
	log.Debugf("received UeGlobalID: %v", ueGlobalId)

	// Gets UE through your global ID
	ue, err := c.getUEWithGlobalID(ctx, ueGlobalId)
	if err != nil {
		return err
	}

	// creates a rsmUeInfo aspect
	rsmUE := &uenib_api.RsmUeInfo{}
	// gets rsmUeInfo aspect from UE
	err = ue.GetAspect(rsmUE)
	if err != nil {
		return err
	}

	// checks if UE exist
	if !c.HasRsmUE(ctx, rsmUE) {
		return errors.NewNotFound(fmt.Sprintf("UE not found - UE: %v", *rsmUE))
	}

	// creates a requisition to delete UE
	req := &uenib_api.DeleteUERequest{
		ID: uenib_api.ID(rsmUE.GetGlobalUeID()),
	}

	// deletes UE
	resp, err := c.uenibClient.DeleteUE(ctx, req)
	if err != nil {
		return err
	}

	log.Debugf("DeleteUE Resp: %v", resp)
	return nil
}

// deletes an RSM UE from specific infrastructure/technology
func (c *Client) DeleteRsmUEWithPreferredID(ctx context.Context, cuNodeID string, preferredType uenib_api.UeIdType, ueConnectionTypeID int64) error {
	log.Debugf("received CUID: %v, preferredType: %v, ueID: %v", cuNodeID, preferredType, ueConnectionTypeID)
	// gets UE
	rsmUE, err := c.GetRsmUEWithPreferredID(ctx, cuNodeID, preferredType, ueConnectionTypeID)
	if err != nil {
		return err
	}
	// deletes UE
	return c.DeleteUE(ctx, rsmUE.GetGlobalUeID())
}

// deletes UE by E2 Node ID
func (c *Client) DeleteUEWithE2NodeID(ctx context.Context, e2NodeID string) error {
	// get UEs with RSM aspect
	rsmUes, err := c.GetRsmUEs(ctx)
	if err != nil {
		return err
	}

	for _, ue := range rsmUes {
		// checks if the UE is connected to a CU or DU
		if ue.GetDuE2NodeId() == e2NodeID || ue.GetCuE2NodeId() == e2NodeID {
			// deletes UE
			err = c.DeleteUE(ctx, ue.GetGlobalUeID())
			if err != nil {
				log.Warn(err)
			}
		}
	}
	return err
}

// gets an RSM UE from specific infrastructure/technology | return RsmUeInfo aspect from UE
func (c *Client) GetRsmUEWithPreferredID(ctx context.Context, cuNodeID string, preferredType uenib_api.UeIdType, ueConnectionTypeID int64) (uenib_api.RsmUeInfo, error) {
	var result uenib_api.RsmUeInfo
	hasUE := false

	// gets RSM UEs
	rsmUes, err := c.GetRsmUEs(ctx)
	if err != nil {
		return uenib_api.RsmUeInfo{}, err
	}

	// for each RSM UEs checks connection technology (is nomenclature right?)
	for _, rsmUE := range rsmUes {
		switch preferredType {
		case uenib_api.UeIdType_UE_ID_TYPE_CU_UE_F1_AP_ID:
			if rsmUE.GetCuE2NodeId() == cuNodeID && rsmUE.GetUeIdList().GetCuUeF1apID().Value == ueConnectionTypeID {
				result = *rsmUE
				hasUE = true
			}
		case uenib_api.UeIdType_UE_ID_TYPE_DU_UE_F1_AP_ID:
			if rsmUE.GetCuE2NodeId() == cuNodeID && rsmUE.GetUeIdList().GetDuUeF1apID().Value == ueConnectionTypeID {
				result = *rsmUE
				hasUE = true
			}
		case uenib_api.UeIdType_UE_ID_TYPE_RAN_UE_NGAP_ID:
			if rsmUE.GetCuE2NodeId() == cuNodeID && rsmUE.GetUeIdList().GetRANUeNgapID().Value == ueConnectionTypeID {
				result = *rsmUE
				hasUE = true
			}
		case uenib_api.UeIdType_UE_ID_TYPE_AMF_UE_NGAP_ID:
			if rsmUE.GetCuE2NodeId() == cuNodeID && rsmUE.GetUeIdList().GetAMFUeNgapID().Value == ueConnectionTypeID {
				result = *rsmUE
				hasUE = true
			}
		case uenib_api.UeIdType_UE_ID_TYPE_ENB_UE_S1_AP_ID:
			if rsmUE.GetCuE2NodeId() == cuNodeID && rsmUE.GetUeIdList().GetEnbUeS1apID().Value == int32(ueConnectionTypeID) {
				result = *rsmUE
				hasUE = true
			}
		default:
			return uenib_api.RsmUeInfo{}, errors.NewNotSupported(fmt.Sprintf("ID type %v is not allowed", preferredType.String()))
		}
	}

	// checks if UE was not found
	if !hasUE {
		return uenib_api.RsmUeInfo{}, errors.NewNotFound(fmt.Sprintf("UE Connection %v, with ID %v, does not exist in CU %v", preferredType.String(), ueConnectionTypeID, cuNodeID))
	}
	return result, nil
}
