package monitoring

import (
	"github.com/gercom-ufpa/iqos-xapp/pkg/broker"
	appConfig "github.com/gercom-ufpa/iqos-xapp/pkg/config"
	"github.com/gercom-ufpa/iqos-xapp/pkg/nib/rnib"
	"github.com/gercom-ufpa/iqos-xapp/pkg/nib/uenib"
	topoapi "github.com/onosproject/onos-api/go/onos/topo"
	e2sm_rsm "github.com/onosproject/onos-e2-sm/servicemodels/e2sm_rsm/v1/e2sm-rsm-ies"
	e2client "github.com/onosproject/onos-ric-sdk-go/pkg/e2/v1beta1"
)

type Monitor struct {
	streamReader           broker.StreamReader
	appConfig              *appConfig.AppConfig
	node                   e2client.Node
	nodeID                 topoapi.ID
	rnibClient             rnib.Client
	uenibClient            uenib.Client
	ricIndEventTriggerType e2sm_rsm.RsmRicindicationTriggerType
}

type Config struct {
	AppConfig        *appConfig.AppConfig
	Node             e2client.Node
	NodeID           topoapi.ID
	StreamReader     broker.StreamReader
	RnibClient       rnib.Client
	UeClient         uenib.Client
	EventTriggerType e2sm_rsm.RsmRicindicationTriggerType
}
