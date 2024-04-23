package config

import (
	app "github.com/onosproject/onos-ric-sdk-go/pkg/config/app/default"
)

// fields in Json file
const (
	ReportPeriodConfigPath      = "/reportPeriod/interval"
	GranularityPeriodConfigPath = "/reportPeriod/granularity"
	E2nodeIDConfigPath          = "/slice/e2nodeid"
	SchedulerConfigPath         = "/slice/scheduler"
	SliceIDConfigPath           = "/slice/sliceid"
	WeightConfigPath            = "/slice/weight"
	SliceTypeConfigPath         = "/slice/type"
)

// xApp Config interface
type Config interface {
	GetReportPeriod() (uint64, error)
	GetGranularityPeriod() (uint64, error)
	GetE2NodeId() (string, error)
	GetScheduler() (string, error)
	GetWeight() (uint64, error)
	GetType() (string, error)
	GetSliceID() (uint64, error)
}

// application configuration
type AppConfig struct {
	appConfig *app.Config
}
