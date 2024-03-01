package e2

import (
	appConfig "github.com/gercom-ufpa/iqos-xapp/pkg/config"
)

// e2 config
type Config struct {
	AppID       string
	AppConfig   *appConfig.AppConfig
	E2tAddress  string
	E2tPort     int
	TopoAddress string
	TopoPort    int
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

// E2 Manager
type Manager struct {
	appConfig *appConfig.AppConfig
}
