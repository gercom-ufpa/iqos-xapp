package ueclient

import (
	"github.com/onosproject/onos-api/go/onos/uenib"
)

type Client struct {
	ueClient uenib.UEServiceClient
	config   Config
}

type Config struct {
	UeNibEndpoint string
	UeNibPort     int
	CertPath      string
	KeyPath       string
}
