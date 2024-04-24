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

func (c *AppConfig) GetE2NodeId() (string, error) {
	e2nodeid, _ := c.appConfig.Get(E2nodeIDConfigPath)
	val, err := configutils.ToString(e2nodeid.Value)
	if err != nil {
		log.Error(err)
		return val, err
	}
	return val, nil
}

// gets granularity period
func (c *AppConfig) GetScheduler() (string, error) {
	scheduler, _ := c.appConfig.Get(SchedulerConfigPath)
	val, err := configutils.ToString(scheduler.Value)
	if err != nil {
		log.Error(err)
		return val, err
	}

	return val, nil
}

func (c *AppConfig) GetType() (string, error) {
	slicetype, _ := c.appConfig.Get(SliceTypeConfigPath)
	val, err := configutils.ToString(slicetype.Value)
	if err != nil {
		log.Error(err)
		return val, err
	}

	return val, nil
}

func (c *AppConfig) GetSliceID() (uint64, error) {
	sliceid, _ := c.appConfig.Get(SliceIDConfigPath)
	val, err := configutils.ToUint64(sliceid.Value)

	if err != nil {
		log.Error(err)
		return 0, err
	}

	return val, nil
}

func (c *AppConfig) GetWeight() (uint64, error) {
	weight, _ := c.appConfig.Get(SliceIDConfigPath)
	val, err := configutils.ToUint64(weight.Value)

	if err != nil {
		log.Error(err)
		return 0, err
	}

	return val, nil
}
