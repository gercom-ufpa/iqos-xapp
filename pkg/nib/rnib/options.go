package rnib

import (
	toposdk "github.com/onosproject/onos-ric-sdk-go/pkg/topo"
)

type Client struct {
	client toposdk.Client
	config Config
}

type Config struct {
	TopoEndpoint string
	TopoPort     int
}
