package manager

import (
	appConfig "github.com/gercom-ufpa/iqos-xapp/pkg/config"
	"github.com/gercom-ufpa/iqos-xapp/pkg/southbound/e2"
	ueclient "github.com/gercom-ufpa/iqos-xapp/pkg/ueclient"
)

// manager configuration
type Config struct {
	AppID         string
	CAPath        string
	KeyPath       string
	CertPath      string
	E2tEndpoint   string
	E2tPort       int
	TopoEndpoint  string
	TopoPort      int
	UeNibEndpoint string
	UeNibPort     int
	ConfigPath    string
	KpmSMName     string
	KpmSMVersion  string
	RsmSMName     string
	RsmSMVersion  string
}

// Manager is an abstract struct for manager
type Manager struct {
	appConfig appConfig.Config
	config    Config
	E2Manager e2.Manager
	UeClient  ueclient.Client
}
