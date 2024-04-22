package manager

import (
	appConfig "github.com/gercom-ufpa/iqos-xapp/pkg/config"
	"github.com/gercom-ufpa/iqos-xapp/pkg/nib/rnib"
	"github.com/gercom-ufpa/iqos-xapp/pkg/nib/uenib"
	nbi "github.com/gercom-ufpa/iqos-xapp/pkg/northbound"
	"github.com/gercom-ufpa/iqos-xapp/pkg/slicing"
	"github.com/gercom-ufpa/iqos-xapp/pkg/southbound/e2"
)

// manager configuration
type Config struct {
	AppID         string
	CAPath        string
	KeyPath       string
	CertPath      string
	E2tEndpoint   string
	E2tPort       int
	GRPCPort      int
	TopoEndpoint  string
	TopoPort      int
	UeNibEndpoint string
	UeNibPort     int
	ConfigPath    string
	KpmSMName     string
	KpmSMVersion  string
	RsmSMName     string
	RsmSMVersion  string
	AckTimer      int
}

// Manager is an abstract struct for manager
type Manager struct {
	appConfig      appConfig.Config
	config         Config
	E2Manager      e2.Manager
	UenibClient    uenib.Client
	RnibClient     rnib.Client
	SlicingManager slicing.Manager
	RsmReqCh       chan *nbi.RsmMsg
}
