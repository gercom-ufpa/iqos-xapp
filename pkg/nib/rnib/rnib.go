package rnib

import (
	"context"

	"github.com/gogo/protobuf/proto"
	topoapi "github.com/onosproject/onos-api/go/onos/topo"
	"github.com/onosproject/onos-lib-go/pkg/logging"
	toposdk "github.com/onosproject/onos-ric-sdk-go/pkg/topo"
)

var log = logging.GetLogger("iqos-xapp", "rnib")

// create a new topo client (R-NIB) | return topo client with your configs
func NewClient(config Config) (Client, error) {
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

// Delete RSMSliceItemList aspect from E2Node
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

// Check if E2Node has RSM Service Model
func (c *Client) HasRSMRANFunction(ctx context.Context, nodeID topoapi.ID, oid string) bool {
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

// Get E2 Node aspects
func (c *Client) GetE2NodeAspects(ctx context.Context, nodeID topoapi.ID) (*topoapi.E2Node, error) {
	// get object by NodeID
	object, err := c.client.Get(ctx, nodeID)
	if err != nil {
		return nil, err
	}

	// Get E2Node data format
	e2Node := &topoapi.E2Node{}

	// Get aspect from object
	err = object.GetAspect(e2Node)
	if err != nil {
		return nil, err
	}

	return e2Node, nil
}
