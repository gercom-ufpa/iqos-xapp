package manager

import (
	appConfig "git.rnp.br/openran/fase-1/gts/gt-iqos/pkg/config"
	"github.com/onosproject/onos-lib-go/pkg/logging"
)

// log initialize
var log = logging.GetLogger("iqos-xapp", "manager")

// new xApp Manager
func NewManager(config *Config) *Manager {
	// initializes xApp configuration
	appCfg, err := appConfig.NewConfig(config.ConfigPath)
	if err != nil {
		log.Warn(err)
	}

	// creates an e2Config

}
