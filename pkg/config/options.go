package config

import (
	app "github.com/onosproject/onos-ric-sdk-go/pkg/config/app/default"
)

// fields in Json file
const (
	// report period config path in json file
	ReportPeriodConfigPath = "/reportPeriod/interval"
	// granularity period config path in json file
	GranularityPeriodConfigPath = "/reportPeriod/granularity"
)

// jsonFormat config
type ConfigJson struct {
	ReportPeriod ReportPeriod `json:"reportPeriod,omitempty"`
}

// report period parameters
type ReportPeriod struct {
	Interval    int64 `json:"interval,omitempty"`
	Granularity int64 `json:"granularity,omitempty"`
}

// xApp Config interface
type Config interface {
	GetReportPeriod() (uint64, error)
	GetGranularityPeriod() (uint64, error)
}

// application configuration
type AppConfig struct {
	appConfig *app.Config
}
