package manager

import (
	appConfig "git.rnp.br/openran/fase-1/gts/gt-iqos/pkg/config"
	"git.rnp.br/openran/fase-1/gts/gt-iqos/pkg/southbound/e2"
)

// manager configuration
type Config struct {
	AppID        string
	CAPath       string
	KeyPath      string
	CertPath     string
	E2tEndpoint  string
	E2tPort      int
	TopoEndpoint string
	TopoPort     int
	ConfigPath   string
	KpmSM
	RsmSM
}

// KPM Service Model config
type KpmSM struct {
	Name    string
	Version string
}

// RSM Service Model config
type RsmSM struct {
	Name    string
	Version string
}

// Manager is an abstract struct for manager
type Manager struct {
	appConfig appConfig.Config
	config    Config
	E2Manager e2.Manager
}
