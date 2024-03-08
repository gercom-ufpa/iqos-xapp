package uenib

import (
	"context"
	"fmt"
	"io"
	"strconv"

	"github.com/onosproject/onos-api/go/onos/uenib"
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
	uenibClient := uenib.NewUEServiceClient(conn)

	return Client{
		uenibClient: uenibClient,
		config:      config,
	}, nil
}

// <--- To Slice Manager module ---->

// Gets UE through your global ID | return an UE
func (c *Client) getUEWithGlobalID(ctx context.Context, ueGlobalId string) (uenib.UE, error) {
	// creates an UE request
	req := &uenib.GetUERequest{
		ID: uenib.ID(ueGlobalId),
	}

	// gets an UE response
	resp, err := c.uenibClient.GetUE(ctx, req)
	if err != nil {
		return uenib.UE{}, err
	}

	// return an UE
	return resp.GetUE(), nil
}

// Gets UEs on the network | return RsmUeInfo aspect of each UE
func (c *Client) GetRsmUEs(ctx context.Context) ([]*uenib.RsmUeInfo, error) {
	// creates a result with RsmUeInfo aspect format
	result := make([]*uenib.RsmUeInfo, 0)

	// creates a list UEs stream
	stream, err := c.uenibClient.ListUEs(ctx, &uenib.ListUERequest{})
	if err != nil {
		return []*uenib.RsmUeInfo{}, err
	}

	// iterate over the stream
	for {
		// get a stream item
		response, err := stream.Recv()
		if err == io.EOF {
			break
		} else if err != nil {
			return []*uenib.RsmUeInfo{}, err
		}
		// gets UE
		ue := response.GetUE()
		// RsmUeInfo aspect
		rsmUE := &uenib.RsmUeInfo{}
		err = ue.GetAspect(rsmUE)
		if err != nil {
			return []*uenib.RsmUeInfo{}, err
		}

		result = append(result, rsmUE)
	}
	return result, nil
}

// Checks if a RSM UE already exists
func (c *Client) HasRsmUE(ctx context.Context, rsmUeAspect *uenib.RsmUeInfo) bool {
	// gets a list of UEs with RSM aspects
	rsmUes, err := c.GetRsmUEs(ctx)
	if err != nil {
		log.Debug("onos-uenib has no UE")
		return false
	}

	// iterate of UEs
	for _, item := range rsmUes {
		// TODO: Is it necessary to check all these values?
		if item.GetGlobalUeID() == rsmUeAspect.GetGlobalUeID() &&
			item.GetUeIdList().GetDuUeF1apID().Value == rsmUeAspect.GetUeIdList().GetDuUeF1apID().Value &&
			item.GetUeIdList().GetCuUeF1apID().Value == rsmUeAspect.GetUeIdList().GetCuUeF1apID().Value &&
			item.GetUeIdList().GetRANUeNgapID().Value == rsmUeAspect.GetUeIdList().GetRANUeNgapID().Value && // for 5G ?
			item.GetUeIdList().GetEnbUeS1apID().Value == rsmUeAspect.GetUeIdList().GetEnbUeS1apID().Value && // for 4G
			item.GetUeIdList().GetAMFUeNgapID().Value == rsmUeAspect.GetUeIdList().GetAMFUeNgapID().Value && // for 5G
			item.GetUeIdList().GetPreferredIDType().String() == rsmUeAspect.GetUeIdList().GetPreferredIDType().String() &&
			item.GetCellGlobalId() == rsmUeAspect.GetCellGlobalId() &&
			item.GetCuE2NodeId() == rsmUeAspect.GetCuE2NodeId() && item.GetDuE2NodeId() == rsmUeAspect.GetDuE2NodeId() {
			return true
		}
	}

	log.Debugf("onos-uenib has UE %v", *rsmUeAspect)
	return false
}

// Adds an UE with RsmUEInfo aspect
func (c *Client) AddRsmUE(ctx context.Context, rsmUeAspect *uenib.RsmUeInfo) error {
	log.Debugf("received ue: %v", rsmUeAspect)

	// checks if UE already exist
	if c.HasRsmUE(ctx, rsmUeAspect) {
		return errors.NewAlreadyExists(fmt.Sprintf("UE already exists - UE: %v", *rsmUeAspect))
	}

	// creates an obj UE
	uenibObj := uenib.UE{
		ID: uenib.ID(rsmUeAspect.GetGlobalUeID()),
	}

	// sets aspect
	uenibObj.SetAspect(rsmUeAspect)

	// creates an UE create request
	req := &uenib.CreateUERequest{
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
func (c *Client) UpdateRsmUE(ctx context.Context, rsmUeAspect *uenib.RsmUeInfo) error {
	// checks if UE already exist
	if !c.HasRsmUE(ctx, rsmUeAspect) {
		return errors.NewNotFound(fmt.Sprintf("UE not found - UE: %v", *rsmUeAspect))
	}

	// creates an obj UE
	uenibObj := uenib.UE{
		ID: uenib.ID(rsmUeAspect.GetGlobalUeID()),
	}

	// sets aspect
	uenibObj.SetAspect(rsmUeAspect)

	// creates an UE create request
	req := &uenib.UpdateUERequest{
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
	rsmUE := &uenib.RsmUeInfo{}
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
	req := &uenib.DeleteUERequest{
		ID: uenib.ID(rsmUE.GetGlobalUeID()),
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
func (c *Client) DeleteRsmUEWithPreferredID(ctx context.Context, cuNodeID string, preferredType uenib.UeIdType, ueConnectionTypeID int64) error {
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
func (c *Client) GetRsmUEWithPreferredID(ctx context.Context, cuNodeID string, preferredType uenib.UeIdType, ueConnectionTypeID int64) (uenib.RsmUeInfo, error) {
	var result uenib.RsmUeInfo
	hasUE := false

	// gets RSM UEs
	rsmUes, err := c.GetRsmUEs(ctx)
	if err != nil {
		return uenib.RsmUeInfo{}, err
	}

	// for each RSM UEs checks connection technology (is nomenclature right?)
	for _, rsmUE := range rsmUes {
		switch preferredType {
		case uenib.UeIdType_UE_ID_TYPE_CU_UE_F1_AP_ID:
			if rsmUE.GetCuE2NodeId() == cuNodeID && rsmUE.GetUeIdList().GetCuUeF1apID().Value == ueConnectionTypeID {
				result = *rsmUE
				hasUE = true
			}
		case uenib.UeIdType_UE_ID_TYPE_DU_UE_F1_AP_ID:
			if rsmUE.GetCuE2NodeId() == cuNodeID && rsmUE.GetUeIdList().GetDuUeF1apID().Value == ueConnectionTypeID {
				result = *rsmUE
				hasUE = true
			}
		case uenib.UeIdType_UE_ID_TYPE_RAN_UE_NGAP_ID:
			if rsmUE.GetCuE2NodeId() == cuNodeID && rsmUE.GetUeIdList().GetRANUeNgapID().Value == ueConnectionTypeID {
				result = *rsmUE
				hasUE = true
			}
		case uenib.UeIdType_UE_ID_TYPE_AMF_UE_NGAP_ID:
			if rsmUE.GetCuE2NodeId() == cuNodeID && rsmUE.GetUeIdList().GetAMFUeNgapID().Value == ueConnectionTypeID {
				result = *rsmUE
				hasUE = true
			}
		case uenib.UeIdType_UE_ID_TYPE_ENB_UE_S1_AP_ID:
			if rsmUE.GetCuE2NodeId() == cuNodeID && rsmUE.GetUeIdList().GetEnbUeS1apID().Value == int32(ueConnectionTypeID) {
				result = *rsmUE
				hasUE = true
			}
		default:
			return uenib.RsmUeInfo{}, errors.NewNotSupported(fmt.Sprintf("ID type %v is not allowed", preferredType.String()))
		}
	}

	// checks if UE was not found
	if !hasUE {
		return uenib.RsmUeInfo{}, errors.NewNotFound(fmt.Sprintf("UE Connection %v, with ID %v, does not exist in CU %v", preferredType.String(), ueConnectionTypeID, cuNodeID))
	}
	return result, nil
}
