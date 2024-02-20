package manager

import (
	appConfig "git.rnp.br/openran/fase-1/gts/gt-iqos/pkg/config"
	"git.rnp.br/openran/fase-1/gts/gt-iqos/pkg/southbound/e2"
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

	// creates an e2Config
	e2config := e2.Config{
		AppID:       config.AppID,
		AppConfig:   appCfg,
		E2tAddress:  config.E2tEndpoint,
		E2tPort:     config.E2tPort,
		TopoAddress: config.TopoEndpoint,
		TopoPort:    config.TopoPort,
		KpmSM: e2.KpmSM{
			Name:    config.KpmSM.Name,
			Version: config.KpmSM.Version,
		},
	}

	// creates managers
	// E2 manager
	e2Manager, err := e2.NewManager(e2config)
	if err != nil {
		log.Warn(err)
	}

	// App Manager
	manager := &Manager{
		appConfig: appCfg,
		config:    config,
		E2Manager: e2Manager,
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

	// starts UEManager Module (TODO)
	// err := mgr.UEManager.Start();
	// if err != nil {
	// 	log.Warnf("Fail to start UEManager: %v", err)
	// 	return err
	// }

	// starts Slice Module (TODO)

	return nil
}

// finalizes xApp processes
func (m *Manager) Close() {
	// TODO
	// syscall.Kill(syscall.Getpid(), syscall.SIGINT)
}
