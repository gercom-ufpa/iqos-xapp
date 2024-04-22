package manager

import (
	"context"

	"github.com/gercom-ufpa/iqos-xapp/pkg/broker"
	appConfig "github.com/gercom-ufpa/iqos-xapp/pkg/config"
	"github.com/gercom-ufpa/iqos-xapp/pkg/nib/rnib"
	"github.com/gercom-ufpa/iqos-xapp/pkg/nib/uenib"
	nbi "github.com/gercom-ufpa/iqos-xapp/pkg/northbound"
	"github.com/gercom-ufpa/iqos-xapp/pkg/slicing"
	"github.com/gercom-ufpa/iqos-xapp/pkg/southbound/e2"
	"github.com/onosproject/onos-lib-go/pkg/logging"
	"github.com/onosproject/onos-lib-go/pkg/northbound"
)

// log initialize
var log = logging.GetLogger("iqos-xapp", "manager")

// New xApp Manager
func NewManager(config Config) *Manager {
	// initializes xApp configuration
	appCfg, err := appConfig.NewConfig(config.ConfigPath)
	if err != nil {
		log.Warn(err)
	}

	// Creates App Clients
	// UE-NIB Client
	uenibConfig := uenib.Config{
		UeNibEndpoint: config.UeNibEndpoint,
		UeNibPort:     config.UeNibPort,
		CertPath:      config.CertPath,
		KeyPath:       config.KeyPath,
	}
	uenibClient, err := uenib.NewClient(context.Background(), uenibConfig)
	if err != nil {
		log.Warn(err)
	}

	// R-NIB Client
	rnibConfig := rnib.Config{
		TopoEndpoint: config.TopoEndpoint,
		TopoPort:     config.TopoPort,
	}
	rnibClient, err := rnib.NewClient(rnibConfig)
	if err != nil {
		log.Warn(err)
	}

	rsmReqCh := make(chan *nbi.RsmMsg)

	// Control messages used by the e2 and slicemgr packages
	slicingCtrlMsgs := e2.SlicingCtrlMsgs{
		CtrlReqChsSliceCreate: make(map[string]chan *e2.CtrlMsg),
		CtrlReqChsSliceUpdate: make(map[string]chan *e2.CtrlMsg),
		CtrlReqChsSliceDelete: make(map[string]chan *e2.CtrlMsg),
		CtrlReqChsUeAssociate: make(map[string]chan *e2.CtrlMsg),
	}

	slicingManager := slicing.NewManager(
		slicing.WithRnibClient(rnibClient),
		slicing.WithUenibClient(uenibClient),
		slicing.WithCtrlReqChs(slicingCtrlMsgs.CtrlReqChsSliceCreate, slicingCtrlMsgs.CtrlReqChsSliceUpdate, slicingCtrlMsgs.CtrlReqChsSliceDelete, slicingCtrlMsgs.CtrlReqChsUeAssociate),
		slicing.WithNbiReqChs(rsmReqCh),
		slicing.WithAckTimer(config.AckTimer),
	)
	// Creates App Managers

	// A1 Manager (or client?? - TODO)

	// E2 manager
	// set Service Models
	serviceModels := e2.ServiceModels{
		KpmSMName:    config.KpmSMName,    // not yet used
		KpmSMVersion: config.KpmSMVersion, // not yet used
		RsmSMName:    config.RsmSMName,
		RsmSMVersion: config.RsmSMVersion,
	}

	// creates the subscriptions broker
	subscriptionBroker := broker.NewBroker()

	e2config := e2.Config{
		AppID:           config.AppID,
		AppConfig:       appCfg,
		E2tAddress:      config.E2tEndpoint,
		E2tPort:         config.E2tPort,
		ServiceModels:   serviceModels,
		Broker:          subscriptionBroker,
		UenibClient:     uenibClient,
		RnibClient:      rnibClient,
		SlicingCtrlMsgs: slicingCtrlMsgs,
	}
	e2Manager, err := e2.NewManager(e2config)
	if err != nil {
		log.Warn(err)
	}

	// App Manager
	manager := &Manager{
		appConfig:      appCfg,
		config:         config,
		E2Manager:      e2Manager,
		SlicingManager: slicingManager,
		UenibClient:    uenibClient,
		RnibClient:     rnibClient,
		RsmReqCh:       rsmReqCh,
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

	// starts Northbound server
	err := mgr.startNorthboundServer()
	if err != nil {
		log.Warn(err)
		return err
	}

	// starts E2 subscriptions
	err = mgr.E2Manager.Start() // TODO
	if err != nil {
		log.Warnf("Fail to start E2 Manager: %v", err)
		return err
	}

	// starts Slice Module (TODO)
	go mgr.SlicingManager.Run(context.Background())

	return nil
}

// finalizes xApp processes
func (m *Manager) Close() {
	// TODO
	// syscall.Kill(syscall.Getpid(), syscall.SIGINT)
}

func (mgr *Manager) startNorthboundServer() error {
	s := northbound.NewServer(northbound.NewServerCfg(
		mgr.config.CAPath,
		mgr.config.KeyPath,
		mgr.config.CertPath,
		int16(mgr.config.GRPCPort),
		true,
		northbound.SecurityConfig{}))

	s.AddService(nbi.NewService(mgr.RnibClient, mgr.UenibClient, mgr.RsmReqCh))

	doneCh := make(chan error)
	go func() {
		err := s.Serve(func(started string) {
			log.Info("Started NBI on ", started)
			close(doneCh)
		})
		if err != nil {
			doneCh <- err
		}
	}()
	return <-doneCh
}
