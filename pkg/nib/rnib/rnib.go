package rnib

import (
	"context"
	"fmt"
	"strings"

	"github.com/atomix/atomix/api/errors"
	"github.com/gogo/protobuf/proto"
	"github.com/onosproject/onos-api/go/onos/rsm"
	topoapi "github.com/onosproject/onos-api/go/onos/topo"
	"github.com/onosproject/onos-lib-go/pkg/logging"
	toposdk "github.com/onosproject/onos-ric-sdk-go/pkg/topo"
)

var log = logging.GetLogger("iqos-xapp", "rnib")

// Creates a new topo client (R-NIB) | return topo client with your configs
func NewClient(config Config) (Client, error) {
	log.Info("Creating a new R-NIB client")
	// new topo client
	sdkClient, err := toposdk.NewClient(
		toposdk.WithTopoAddress(
			config.TopoEndpoint,
			config.TopoPort,
		),
	)
	if err != nil {
		return Client{}, err
	}

	return Client{
		client: sdkClient,
		config: config,
	}, nil
}

// Deletes RSMSliceItemList aspect from E2Node
func (c *Client) DeleteRsmSliceList(ctx context.Context, nodeID topoapi.ID) error {
	// get topo object/E2Node
	object, err := c.client.Get(ctx, nodeID)
	if err != nil {
		return err
	}

	// get aspect name from protobuf message
	aspectKey := proto.MessageName(&topoapi.RSMSliceItemList{})

	// delete aspect from E2Node
	delete(object.Aspects, aspectKey)

	// update E2Node
	err = c.client.Update(ctx, object)
	if err != nil {
		return err
	}

	return nil
}

// Checks if E2Node has a RAN Function by its OID
func (c *Client) HasRANFunction(ctx context.Context, nodeID topoapi.ID, oid string) bool {
	// get E2Nodes aspects
	e2Node, err := c.GetE2NodeAspects(ctx, nodeID)
	if err != nil {
		log.Warn(err)
		return false
	}

	// check if Service Model is present
	for _, sm := range e2Node.GetServiceModels() {
		if sm.OID == oid {
			return true
		}
	}
	return false
}

// Gets E2 Node aspects
func (c *Client) GetE2NodeAspects(ctx context.Context, nodeID topoapi.ID) (*topoapi.E2Node, error) {
	// get object by NodeID
	object, err := c.client.Get(ctx, nodeID)
	if err != nil {
		return nil, err
	}

	log.Debug(object.GetEntity().GetKindID())
	log.Debug(object.GetKind().GetName())
	log.Info("test")

	// check if object is an E2 Node
	if object.GetEntity().GetKindID() != "e2node" {
		return nil, errors.NewInvalid("The topo object with ID %v is not an E2 Node", nodeID)
	}

	// Get E2Node aspect format
	e2Node := &topoapi.E2Node{}

	// Get aspect from object
	err = object.GetAspect(e2Node)
	if err != nil {
		return nil, err
	}

	return e2Node, nil
}

// Gets RSMSlicingItem aspects on node | return RSMSlicingItem aspects per slice
func (c *Client) GetRsmSliceItemAspects(ctx context.Context, nodeID topoapi.ID) ([]*topoapi.RSMSlicingItem, error) {
	// get RSMSlicingItem/RSM Slices on node
	rsmSliceList, err := c.GetRsmSliceListAspect(ctx, nodeID)
	if err != nil {
		return nil, errors.NewNotFound("node %v has no slices", nodeID)
	}

	return rsmSliceList.GetRsmSliceList(), nil
}

// Gets RsmSliceItemList aspect on Node | return RsmSliceItemList aspect per slice
func (c *Client) GetRsmSliceListAspect(ctx context.Context, nodeID topoapi.ID) (*topoapi.RSMSliceItemList, error) {
	// gets a topo object by ID
	object, err := c.client.Get(ctx, nodeID)
	if err != nil {
		return nil, err
	}

	// RsmSliceItemList aspect
	aspect := &topoapi.RSMSliceItemList{}
	err = object.GetAspect(aspect)
	if err != nil {
		return nil, err
	}

	return aspect, nil
}

// Gets slice's RSMSlicingItem aspect on node | return RSMSlicingItem
func (c *Client) GetRsmSliceItemAspect(ctx context.Context, nodeID topoapi.ID, sliceID string, sliceType rsm.SliceType) (*topoapi.RSMSlicingItem, error) {
	// get RSMSlicingItem/RSM Slices on node
	rsmSliceList, err := c.GetRsmSliceListAspect(ctx, nodeID)
	if err != nil {
		return nil, errors.NewNotFound("node %v has no slices", nodeID)
	}

	var topoSliceType topoapi.RSMSliceType
	// check if slice type is supported
	switch sliceType {
	case rsm.SliceType_SLICE_TYPE_DL_SLICE: // DL slice
		topoSliceType = topoapi.RSMSliceType_SLICE_TYPE_DL_SLICE
	case rsm.SliceType_SLICE_TYPE_UL_SLICE: // UL slice
		topoSliceType = topoapi.RSMSliceType_SLICE_TYPE_UL_SLICE
	default:
		return nil, errors.NewNotSupported(fmt.Sprintf("slice type %v does not supported", sliceType.String()))
	}

	// gets slice by ID and type
	for _, slice := range rsmSliceList.GetRsmSliceList() {
		if slice.GetID() == sliceID && slice.GetSliceType() == topoSliceType {
			return slice, nil
		}
	}

	return nil, errors.NewNotFound("node %v does not have slice %v (%v)", nodeID, sliceID, sliceType.String())
}

// Deletes RsmSliceItemAspect by Node and Slice ID
func (c *Client) DeleteRsmSliceItemAspect(ctx context.Context, nodeID topoapi.ID, sliceID string) error {
	// get Rsm Slices
	rsmSliceList, err := c.GetRsmSliceListAspect(ctx, nodeID)
	if err != nil {
		return errors.NewNotFound("node %v has no slices", nodeID)
	}

	// finds the RSM slice in rsmSliceList and delete it
	for i := 0; i < len(rsmSliceList.GetRsmSliceList()); i++ {
		if rsmSliceList.GetRsmSliceList()[i].GetID() == sliceID {
			rsmSliceList.RsmSliceList = append(rsmSliceList.RsmSliceList[:i], rsmSliceList.RsmSliceList[i+1:]...)
			break
		}
	}

	// updates slice aspect
	err = c.SetRsmSliceListAspect(ctx, nodeID, rsmSliceList)
	if err != nil {
		return err
	}
	return nil
}

// Updates the RsmSliceListAspect aspect of the node
func (c *Client) SetRsmSliceListAspect(ctx context.Context, nodeID topoapi.ID, rsmSliceList *topoapi.RSMSliceItemList) error {
	// gets topo object by ID
	object, err := c.client.Get(ctx, nodeID)
	if err != nil {
		return err
	}

	// defines the set of new rsm slices to node
	err = object.SetAspect(rsmSliceList)
	if err != nil {
		return err
	}
	// updates node informations
	err = c.client.Update(ctx, object)
	if err != nil {
		return err
	}

	return nil
}

// Adds the RsmSliceListAspect aspect of the node
func (c *Client) AddRsmSliceItemAspect(ctx context.Context, nodeID topoapi.ID, RsmSlice *topoapi.RSMSlicingItem) error {
	// gets RSM slice list
	rsmSliceList, err := c.GetRsmSliceListAspect(ctx, nodeID)
	// creates a new rsm slice item list if not found
	if err != nil {
		rsmSliceList = &topoapi.RSMSliceItemList{
			RsmSliceList: make([]*topoapi.RSMSlicingItem, 0),
		}
	}

	// adds the Rsm slice to list
	rsmSliceList.RsmSliceList = append(rsmSliceList.RsmSliceList, RsmSlice)
	// sets RSMSliceItemList aspect
	err = c.SetRsmSliceListAspect(ctx, nodeID, rsmSliceList)
	if err != nil {
		return err
	}
	return nil
}

// Check if has a Rsm Slice on rsmSliceList | return true if exist or false if not
func (c *Client) HasRsmSliceItemAspect(ctx context.Context, nodeID topoapi.ID, sliceID string, sliceType rsm.SliceType) bool {
	// gets list of Rsm slices
	rsmSliceList, err := c.GetRsmSliceListAspect(ctx, nodeID)
	if err != nil {
		return false
	}

	// check slice type
	var topoSliceType topoapi.RSMSliceType
	switch sliceType {
	case rsm.SliceType_SLICE_TYPE_DL_SLICE:
		topoSliceType = topoapi.RSMSliceType_SLICE_TYPE_DL_SLICE
	case rsm.SliceType_SLICE_TYPE_UL_SLICE:
		topoSliceType = topoapi.RSMSliceType_SLICE_TYPE_UL_SLICE
	default:
		return false
	}

	// check if slice is in list
	for _, slice := range rsmSliceList.GetRsmSliceList() {
		if slice.GetID() == sliceID && slice.GetSliceType() == topoSliceType {
			return true
		}
	}

	return false
}

// Gets Supported Slicing Config Types
func (c *Client) GetSupportedSlicingConfigTypes(ctx context.Context, nodeID topoapi.ID) ([]*topoapi.RSMSupportedSlicingConfigItem, error) {
	// make a list of configs item supported by slice
	result := make([]*topoapi.RSMSupportedSlicingConfigItem, 0)
	e2Node, err := c.GetE2NodeAspects(ctx, nodeID)
	if err != nil {
		return nil, err
	}

	// For Service Models supporteds
	for smName, sm := range e2Node.GetServiceModels() {
		for _, ranFunc := range sm.GetRanFunctions() { // For Ran Functions supporteds
			// gets a Rsm RAN function data model
			rsmRanFunc := &topoapi.RSMRanFunction{}
			// check if rsmRanFunction exist in proto
			err = proto.Unmarshal(ranFunc.GetValue(), rsmRanFunc)
			if err != nil {
				log.Debugf("RanFunction for SM - %v, URL - %v does not have RSM RAN Function Description:\n%v", smName, ranFunc.GetTypeUrl(), err)
				continue
			}

			// for each slice capacity
			for _, cap := range rsmRanFunc.GetRicSlicingNodeCapabilityList() {
				for i := 0; i < len(cap.GetSupportedConfig()); i++ {
					// adds configurations supported by slice (e.g. CREATE, UPDATE..)
					result = append(result, cap.GetSupportedConfig()[i])
				}
			}
		}
	}
	return result, nil
}

// Watch E2 Connections (E2T <-> E2Node)
func (c *Client) WatchE2Connections(ctx context.Context, ch chan topoapi.Event) error {
	err := c.client.Watch(ctx, ch, toposdk.WithWatchFilters(getControlRelationFilter()))
	if err != nil {
		return err
	}
	return nil
}

// Gets a control relation filter
func getControlRelationFilter() *topoapi.Filters {
	filter := &topoapi.Filters{
		KindFilter: &topoapi.Filter{
			Filter: &topoapi.Filter_Equal_{
				Equal_: &topoapi.EqualFilter{
					Value: topoapi.CONTROLS,
				},
			},
		},
	}
	return filter
}

// Gets target DU node ID by CU node ID | return DU ID connected to CU
func (c *Client) GetTargetDUE2NodeID(ctx context.Context, cuE2NodeID topoapi.ID) (topoapi.ID, error) {
	// TODO: When auto-discovery comes in, it should be changed

	// gets topo objects
	objects, err := c.client.List(ctx)
	if err != nil {
		return "", err
	}

	for _, obj := range objects {
		log.Debugf("Relation: %v", obj.GetEntity())
		if obj.GetEntity() != nil && obj.GetEntity().GetKindID() == topoapi.E2NODE { // check if the object is an E2 Node
			if cuE2NodeID != obj.GetID() { // if nodeID is not from the CU itself
				// get part 1 and 2 of the CU NodeID
				nodeID := fmt.Sprintf("%s/%s", strings.Split(string(cuE2NodeID), "/")[0], strings.Split(string(cuE2NodeID), "/")[1])
				// get part 1 and 2 of the DU NodeID
				tgtNodeID := fmt.Sprintf("%s/%s", strings.Split(string(obj.GetID()), "/")[0], strings.Split(string(obj.GetID()), "/")[1])
				if nodeID == tgtNodeID { // if DU contain part of CU ID, return DU ID.
					return obj.GetID(), nil
				}
			}
		}
	}

	return "", errors.NewNotFound(fmt.Sprintf("DU-ID not found (CU-ID: %v)", cuE2NodeID))
}

// Gets source CU node ID by DU node ID | return CU ID connected to DU
func (c *Client) GetSourceCUE2NodeID(ctx context.Context, duE2NodeID topoapi.ID) (topoapi.ID, error) {
	// TODO: When auto-discovery comes in, it should be changed

	// gets topo objects
	objects, err := c.client.List(ctx)
	if err != nil {
		return "", err
	}

	for _, obj := range objects {
		log.Debugf("Relation: %v", obj.GetEntity())
		if obj.GetEntity() != nil && obj.GetEntity().GetKindID() == topoapi.E2NODE { // check if the object is an E2 Node
			if duE2NodeID != obj.GetID() { // if nodeID is not from the DU itself
				// get part 1 and 2 of the DU NodeID
				nodeID := fmt.Sprintf("%s/%s", strings.Split(string(duE2NodeID), "/")[0], strings.Split(string(duE2NodeID), "/")[1])
				// get part 1 and 2 of the CU NodeID
				tgtNodeID := fmt.Sprintf("%s/%s", strings.Split(string(obj.GetID()), "/")[0], strings.Split(string(obj.GetID()), "/")[1])
				if nodeID == tgtNodeID { // if CU contain part of DU ID, return CU ID.
					return obj.GetID(), nil
				}
			}
		}
	}

	return "", errors.NewNotFound(fmt.Sprintf("CU-ID not found (DU-ID: %v)", duE2NodeID))
}

// Gets RSMSlicingItem aspects on all DUs | return list of RSMSlicingItem aspects per DUs
func (c *Client) GetRSMSliceItemAspectsForAllDUs(ctx context.Context) (map[string][]*topoapi.RSMSlicingItem, error) {
	// creates a list with RSMSlicingItem aspect data format
	dusRsmAspectList := make(map[string][]*topoapi.RSMSlicingItem)
	// gets topo object list
	objects, err := c.client.List(ctx)
	if err != nil {
		return nil, err
	}

	for _, obj := range objects {
		if obj.GetEntity() != nil && obj.GetEntity().GetKindID() == topoapi.E2NODE { // check if the object is an E2 Node
			if len(strings.Split(string(obj.GetID()), "/")) == 4 && strings.Split(string(obj.GetID()), "/")[2] == "3" { // check if the E2Node is a DU
				// gets RSMSlicingItem aspect data format
				rsmAspect := &topoapi.RSMSliceItemList{}
				err = obj.GetAspect(rsmAspect)
				if err != nil {
					return nil, err
				}
				// adds DU's RSM aspect to list
				dusRsmAspectList[string(obj.GetID())] = rsmAspect.GetRsmSliceList()
			}
		}
	}

	return dusRsmAspectList, nil
}
