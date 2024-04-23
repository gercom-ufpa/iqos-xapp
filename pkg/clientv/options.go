package clientv

import (
	"fmt"

	rsmapi "github.com/onosproject/onos-api/go/onos/rsm"
)

const (
	TLSCaCrtPath        = "/home/mth/iqos-xapp/deploys/helm-chart/iqos-chart/files/certs/tls.cacert"
	TLSCrtPath          = "/home/mth/iqos-xapp/deploys/helm-chart/iqos-chart/files/certs/tls.crt"
	TLSKeyPath          = "/home/mth/iqos-xapp/deploys/helm-chart/iqos-chart/files/certs/tls.key"
	E2nodeIDConfigPath  = "/slice/e2nodeid"
	SchedulerConfigPath = "/slice/scheduler"
	SliceIDConfigPath   = "/slice/sliceid"
	WeightConfigPath    = "/slice/weight"
	SliceTypeConfigPath = "/slice/type"
)

func GetSliceScheduler(scheduler string) rsmapi.SchedulerType {

	var schedulerTypeField rsmapi.SchedulerType
	switch scheduler {
	case "RR":
		schedulerTypeField = rsmapi.SchedulerType_SCHEDULER_TYPE_ROUND_ROBIN
	case "PF":
		schedulerTypeField = rsmapi.SchedulerType_SCHEDULER_TYPE_PROPORTIONALLY_FAIR
	case "QoS":
		schedulerTypeField = rsmapi.SchedulerType_SCHEDULER_TYPE_QOS_BASED
	default:
		schedulerTypeField = rsmapi.SchedulerType_SCHEDULER_TYPE_ROUND_ROBIN
	}
	fmt.Printf("%s\n", schedulerTypeField)
	return schedulerTypeField
}
func GetSliceType(sliceType string) rsmapi.SliceType {

	var sliceTypeField rsmapi.SliceType
	switch sliceType {
	case "DL":
		sliceTypeField = rsmapi.SliceType_SLICE_TYPE_DL_SLICE
	case "UL":
		sliceTypeField = rsmapi.SliceType_SLICE_TYPE_UL_SLICE
	default:
		sliceTypeField = rsmapi.SliceType_SLICE_TYPE_DL_SLICE
	}
	fmt.Printf("%s\n", sliceTypeField)
	return sliceTypeField
}
