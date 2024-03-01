package uemgr

import "github.com/onosproject/onos-api/go/onos/uenib"

// default aspect types to get of UEs
var defaultAspectTypes = []string{}

type Manager struct {
	ueClient uenib.UEServiceClient
}

type Config struct {
	UeNibEndpoint string
	UeNibPort     int
}

type UE struct {
	ID          string
	aspectTypes []string
}

type UEs []UE
