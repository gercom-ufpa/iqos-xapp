package e2

import (
	"github.com/onosproject/onos-lib-go/pkg/logging"
	e2client "github.com/onosproject/onos-ric-sdk-go/pkg/e2/v1beta1"
)

// log initialize
var log = logging.GetLogger("iqos-xapp", "e2")

func NewManager(config Config) (Manager, error) {
	log.Info("Starting E2 Manager")

	// creates a E2 Client
	e2Client := e2client.NewClient(
		e2client.WithAppID(e2client.AppID(config.AppID)),
		e2client.WithE2TAddress( // sets E2T address
			config.E2tAddress,
			config.E2tPort,
		),
		e2client.WithServiceModel( // sets Service Model
			e2client.ServiceModelName(config.ServiceModels.RsmSMName),
			e2client.ServiceModelVersion(config.ServiceModels.RsmSMVersion),
		),
		e2client.WithEncoding(e2client.ProtoEncoding), // sets enconding
	)

	return Manager{
		AppID:           config.AppID,
		e2Client:        e2Client,
		rnibClient:      config.RnibClient,
		uenibClient:     config.UenibClient,
		ServiceModels:   config.ServiceModels,
		streams:         config.Broker,
		appConfig:       config.AppConfig,
		SlicingCtrlMsgs: config.SlicingCtrlMsgs,
	}, nil
}

// TODO
func (mgr *Manager) Start() error {
	return nil
}
