package main

import (
	"github.com/onosproject/onos-lib-go/pkg/logging"
)

// initialize log var (app_name, package, subpackage...)
var log = logging.GetLogger("iqos-xapp", "main")

func main() {
	// defines log level
	log.SetLevel(logging.DebugLevel)

	// initial app message
	log.Info("Starting IQoS xAPP")

}
