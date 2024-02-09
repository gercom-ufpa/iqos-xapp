package manager

import appConfig "git.rnp.br/openran/fase-1/gts/gt-iqos/pkg/config"

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
	SMName       string
	SMVersion    string
}

// Manager is an abstract struct for manager
type Manager struct {
	appConfig appConfig.Config
	config    Config
}
