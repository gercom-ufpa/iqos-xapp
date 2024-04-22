// SPDX-FileCopyrightText: 2020-present Open Networking Foundation <info@opennetworking.org>
//
// SPDX-License-Identifier: Apache-2.0

package northbound

import (
	"context"

	"github.com/gercom-ufpa/iqos-xapp/pkg/nib/rnib"
	"github.com/gercom-ufpa/iqos-xapp/pkg/nib/uenib"
	rsmapi "github.com/onosproject/onos-api/go/onos/rsm"
	topoapi "github.com/onosproject/onos-api/go/onos/topo"
	"github.com/onosproject/onos-lib-go/pkg/logging/service"
	"google.golang.org/grpc"
)

// Creates a new service with the provided clients and the RSM messaging channel.
func NewService(rnibClient rnib.Client, uenibClient uenib.Client, rsmReqCh chan *RsmMsg) service.Service {
	return &Service{
		rnibClient:  rnibClient,
		uenibClient: uenibClient,
		rsmReqCh:    rsmReqCh,
	}
}

// Register registers the service on the provided gRPC server.
func (s Service) Register(r *grpc.Server) {
	server := &Server{
		rnibClient:  s.rnibClient,
		uenibClient: s.uenibClient,
		rsmReqCh:    s.rsmReqCh,
	}
	rsmapi.RegisterRsmServer(r, server)
}

// Creates a new slice using the given parameters.
func (s Server) CreateSlice(_ context.Context, request *rsmapi.CreateSliceRequest) (*rsmapi.CreateSliceResponse, error) {
	// Create a confirmation channel.
	ackCh := make(chan Ack)
	// Creates an RSM message.
	msg := &RsmMsg{
		NodeID:  topoapi.ID(request.E2NodeId),
		Message: request,
		AckCh:   ackCh,
	}
	//Sends the message to the RSM messaging channel in a goroutine.
	go func(msg *RsmMsg) {
		s.rsmReqCh <- msg
	}(msg)
	// Wait for confirmation of the operation.
	ack := <-ackCh
	// Returns the response.
	return &rsmapi.CreateSliceResponse{
		Ack: &rsmapi.Ack{
			Success: ack.Success,
			Cause:   ack.Reason,
		},
	}, nil
}

func (s Server) UpdateSlice(_ context.Context, request *rsmapi.UpdateSliceRequest) (*rsmapi.UpdateSliceResponse, error) {
	ackCh := make(chan Ack)
	msg := &RsmMsg{
		NodeID:  topoapi.ID(request.E2NodeId),
		Message: request,
		AckCh:   ackCh,
	}
	go func(msg *RsmMsg) {
		s.rsmReqCh <- msg
	}(msg)

	ack := <-ackCh
	return &rsmapi.UpdateSliceResponse{
		Ack: &rsmapi.Ack{
			Success: ack.Success,
			Cause:   ack.Reason,
		},
	}, nil
}

func (s Server) DeleteSlice(_ context.Context, request *rsmapi.DeleteSliceRequest) (*rsmapi.DeleteSliceResponse, error) {
	ackCh := make(chan Ack)
	msg := &RsmMsg{
		NodeID:  topoapi.ID(request.E2NodeId),
		Message: request,
		AckCh:   ackCh,
	}
	go func(msg *RsmMsg) {
		s.rsmReqCh <- msg
	}(msg)

	ack := <-ackCh
	return &rsmapi.DeleteSliceResponse{
		Ack: &rsmapi.Ack{
			Success: ack.Success,
			Cause:   ack.Reason,
		},
	}, nil
}

func (s Server) SetUeSliceAssociation(_ context.Context, request *rsmapi.SetUeSliceAssociationRequest) (*rsmapi.SetUeSliceAssociationResponse, error) {
	ackCh := make(chan Ack)
	msg := &RsmMsg{
		NodeID:  topoapi.ID(request.E2NodeId),
		Message: request,
		AckCh:   ackCh,
	}
	go func(msg *RsmMsg) {
		s.rsmReqCh <- msg
	}(msg)

	ack := <-ackCh
	return &rsmapi.SetUeSliceAssociationResponse{
		Ack: &rsmapi.Ack{
			Success: ack.Success,
			Cause:   ack.Reason,
		},
	}, nil
}
