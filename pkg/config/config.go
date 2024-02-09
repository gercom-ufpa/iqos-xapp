package config

import (
	"context"

	"github.com/onosproject/onos-lib-go/pkg/logging"
	app "github.com/onosproject/onos-ric-sdk-go/pkg/config/app/default"
	event "github.com/onosproject/onos-ric-sdk-go/pkg/config/event"
	configurable "github.com/onosproject/onos-ric-sdk-go/pkg/config/registry"
	configutils "github.com/onosproject/onos-ric-sdk-go/pkg/config/utils"
)

var log = logging.GetLogger("iqos-xapp", "config")

// initializes the xApp config
func NewConfig(configPath string) (*AppConfig, error) {
	appConfig, err := configurable.RegisterConfigurable(configPath, &configurable.RegisterRequest{})
	if err != nil {
		return nil, err
	}

	cfg := &AppConfig{
		appConfig: appConfig.Config.(*app.Config),
	}

	return cfg, nil
}

// watches config changes
func (c *AppConfig) Watch(ctx context.Context, ch chan event.Event) error {
	err := c.appConfig.Watch(ctx, ch)
	if err != nil {
		return err
	}
	return nil
}

// gets report period
func (c *AppConfig) GetReportPeriod() (uint64, error) {
	interval, _ := c.appConfig.Get(ReportPeriodConfigPath)
	val, err := configutils.ToUint64(interval.Value)
	if err != nil {
		log.Error(err)
		return 0, err
	}
	return val, nil
}

// gets granularity period
func (c *AppConfig) GetGranularityPeriod() (uint64, error) {
	granularity, _ := c.appConfig.Get(GranularityPeriodConfigPath)
	val, err := configutils.ToUint64(granularity.Value)
	if err != nil {
		log.Error(err)
		return 0, err
	}
	return val, nil
}
