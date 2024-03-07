package e2

import e2api "github.com/onosproject/onos-api/go/onos/e2t/e2/v1beta1"

type Ack struct {
	Success bool
	Reason  string
}

type CtrlMsg struct {
	CtrlMsg *e2api.ControlMessage
	AckCh   chan Ack
}
