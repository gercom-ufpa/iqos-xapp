package e2

import "github.com/onosproject/onos-lib-go/pkg/logging"

// log initialize
var log = logging.GetLogger("iqos-xapp", "e2")

func NewManager(config Config) (Manager, error) {
	log.Info("Starting E2 Manager")
	return Manager{}, nil
}

// TODO
func (mgr *Manager) Start() error {
	return nil
}
