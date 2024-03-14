// SPDX-FileCopyrightText: 2020-present Open Networking Foundation <info@opennetworking.org>
//
// SPDX-License-Identifier: Apache-2.0

package northbound

import (
	topoapi "github.com/onosproject/onos-api/go/onos/topo"
	"github.com/onosproject/onos-rsm/pkg/nib/rnib"
	"github.com/onosproject/onos-rsm/pkg/nib/uenib"
)

// Confirmation message for an operation, containing a boolean
// below if the operation was successful and a string explaining the reason in case of failure
type Ack struct {
	Success bool
	Reason  string
}

// Definition of the fields NodeID, which is an identifier for
// a node in the network topology, Message, which is an interface for a generic message,
// and AckCh, which is a channel to receive a confirmation (Ack) of the operation
type RsmMsg struct {
	NodeID  topoapi.ID
	Message interface{}
	AckCh   chan Ack
}

// This Service struct encapsulates the clients for rnib and uenib, as well as a channel
// for receiving RSM messages. It is intended to be used to provide RSM-related services.
type Service struct {
	rnibClient  rnib.TopoClient
	uenibClient uenib.Client
	rsmReqCh    chan *RsmMsg
}

// Create a gRPC server to serve RPC requests
type Server struct {
	rnibClient  rnib.TopoClient
	uenibClient uenib.Client
	rsmReqCh    chan *RsmMsg
}
