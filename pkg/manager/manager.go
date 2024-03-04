package manager

import (
	"context"

	appConfig "github.com/gercom-ufpa/iqos-xapp/pkg/config"
	"github.com/gercom-ufpa/iqos-xapp/pkg/southbound/e2"
	ueclient "github.com/gercom-ufpa/iqos-xapp/pkg/ueclient"
	"github.com/onosproject/onos-lib-go/pkg/logging"
)

// log initialize
var log = logging.GetLogger("iqos-xapp", "manager")

// new xApp Manager
func NewManager(config Config) *Manager {
	// initializes xApp configuration
	appCfg, err := appConfig.NewConfig(config.ConfigPath)
	if err != nil {
		log.Warn(err)
	}

	// Creates App Clients
	// UE-NIB Client
	ueClientConfig := ueclient.Config{
		UeNibEndpoint: config.UeNibEndpoint,
		UeNibPort:     config.UeNibPort,
		CertPath:      config.CertPath,
		KeyPath:       config.KeyPath,
	}
	ueClient, err := ueclient.NewClient(context.Background(), ueClientConfig)
	if err != nil {
		log.Warn(err)
	}

	// R-NIB Manager (TODO)

	// A1 Manager (or client?? - TODO)

	// E2 manager
	e2config := e2.Config{
		AppID:        config.AppID,
		AppConfig:    appCfg,
		E2tAddress:   config.E2tEndpoint,
		E2tPort:      config.E2tPort,
		TopoAddress:  config.TopoEndpoint,
		TopoPort:     config.TopoPort,
		KpmSMName:    config.KpmSMName,
		KpmSMVersion: config.KpmSMVersion,
		RsmSMName:    config.RsmSMName,
		RsmSMVersion: config.RsmSMVersion,
	}
	e2Manager, err := e2.NewManager(e2config)
	if err != nil {
		log.Warn(err)
	}

	// App Manager
	manager := &Manager{
		appConfig: appCfg,
		config:    config,
		E2Manager: e2Manager,
		UeClient:  ueClient,
	}

	return manager
}

// run xApp
func (mgr *Manager) Run() {
	if err := mgr.start(); err != nil {
		log.Errorf("Unable when starting Manager: %v", err)
	}
}

// starts xAPP processes
func (mgr *Manager) start() error {
	// starts E2 subscriptions
	err := mgr.E2Manager.Start() // TODO
	if err != nil {
		log.Warnf("Fail to start E2 Manager: %v", err)
		return err
	}

	// starts Slice Module (TODO)

	return nil
}

// finalizes xApp processes
func (m *Manager) Close() {
	// TODO
	// syscall.Kill(syscall.Getpid(), syscall.SIGINT)
}
