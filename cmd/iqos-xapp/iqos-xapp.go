package main

import (
	"os"
	"os/signal"
	"syscall"

	"github.com/gercom-ufpa/iqos-xapp/pkg/manager"
	"github.com/onosproject/onos-lib-go/pkg/logging"
)

// initialize log var (app_name, package, subpackage...)
var log = logging.GetLogger("iqos-xapp", "main")

func main() {
	// defines log level
	log.SetLevel(logging.DebugLevel)

	// initial app message
	log.Info("Starting IQoS xAPP")

	// set manager configuration
	cfg := manager.Config{
		AppID:        "iqos-xapp",
		CAPath:       "/etc/iqos-xapp/certs/ca.pem",
		KeyPath:      "/etc/iqos-xapp/certs/tls.key",
		CertPath:     "/etc/iqos-xapp/certs/tls.crt",
		E2tEndpoint:  "onos-e2t",
		E2tPort:      5150,
		TopoEndpoint: "onos-topo",
		TopoPort:     5150,
		ConfigPath:   "/etc/iqos-xapp/config/config.json",
		KpmSM: manager.KpmSM{
			Name:    "oran-e2sm-kpm",
			Version: "v2",
		},
	}

	// creates the xApp manager
	mgr := manager.NewManager(cfg)

	// run APP manager
	mgr.Run()

	// configures a shutdown signal for xApp
	killSignal := make(chan os.Signal, 1)
	signal.Notify(killSignal, os.Interrupt, syscall.SIGTERM, syscall.SIGINT)
	log.Debug("xApp received a shutdown signal ", <-killSignal)

	// finalizes xApp processes
	mgr.Close()
}
