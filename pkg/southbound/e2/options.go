package e2

import (
	"github.com/gercom-ufpa/iqos-xapp/pkg/broker"
	appConfig "github.com/gercom-ufpa/iqos-xapp/pkg/config"
	"github.com/gercom-ufpa/iqos-xapp/pkg/nib/rnib"
	"github.com/gercom-ufpa/iqos-xapp/pkg/nib/uenib"
	e2api "github.com/onosproject/onos-api/go/onos/e2t/e2/v1beta1"
	e2client "github.com/onosproject/onos-ric-sdk-go/pkg/e2/v1beta1"
)

// Service Models' OIDs
const (
	smRsmOID = "1.3.6.1.4.1.53148.1.1.2.102" // RSM-SM
	smKpmOID = "1.3.6.1.4.1.53148.1.2.2.2"   // KPM-SM
)

// e2 config
type Config struct {
	AppID       string
	AppConfig   *appConfig.AppConfig
	E2tAddress  string
	E2tPort     int
	UenibClient uenib.Client
	RnibClient  rnib.Client
	Broker      broker.Broker
	ServiceModels
	SlicingCtrlMsgs
}

// E2 Manager
type Manager struct {
	AppID       string
	appConfig   *appConfig.AppConfig
	e2Client    e2client.Client
	rnibClient  rnib.Client
	uenibClient uenib.Client
	streams     broker.Broker // TODO
	ServiceModels
	SlicingCtrlMsgs
}

// Service Models available
type ServiceModels struct {
	KpmSMName    string
	KpmSMVersion string
	RsmSMName    string
	RsmSMVersion string
}

// Ack message
type Ack struct {
	Success bool
	Reason  string
}

// Control message
type CtrlMsg struct {
	CtrlMsg *e2api.ControlMessage
	AckCh   chan Ack
}

// Slice control messages/Responses
type SlicingCtrlMsgs struct {
	CtrlReqChsSliceCreate map[string]chan *CtrlMsg
	CtrlReqChsSliceUpdate map[string]chan *CtrlMsg
	CtrlReqChsSliceDelete map[string]chan *CtrlMsg
	CtrlReqChsUeAssociate map[string]chan *CtrlMsg
}
