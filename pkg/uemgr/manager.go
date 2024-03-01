package uemgr

import (
	"context"
	"io"
	"strconv"

	"github.com/onosproject/onos-api/go/onos/uenib"
	"github.com/onosproject/onos-lib-go/pkg/grpc/retry"
	"github.com/onosproject/onos-lib-go/pkg/logging"
	"github.com/onosproject/onos-ric-sdk-go/pkg/e2/creds"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

var log = logging.GetLogger("qmai", "uemgr")

// creates a new UE Manager
func NewManager(config Config) (Manager, error) {
	// onos-uenib address
	ueNibServiceAddress := config.UeNibEndpoint + ":" + strconv.Itoa(config.UeNibPort)
	// connects to UE NIB service
	conn, err := connectUeNibServiceHost(ueNibServiceAddress)
	if err != nil {
		return Manager{}, err
	}
	// creates a ue-nib client
	ueClient := uenib.CreateUEServiceClient(conn)

	return Manager{
		ueClient: ueClient,
	}, nil
}

// starts UE Manager
func (m *Manager) Start() error {
	// go routine
	go func() {
		ctx, cancel := context.WithCancel(context.Background())
		defer cancel()
		err := m.watchUEConnections(ctx)
		if err != nil {
			log.Warn(err)
			return
		}
	}()

	return nil
}

// watch UEs changes
func (m *Manager) watchUEConnections(ctx context.Context) error {
	// list UEs
	m.listUEs(ctx)

	// watch changes
	log.Info("Starting to watch the UEs changes")
	stream, err := m.ueClient.WatchUEs(ctx, &uenib.WatchUERequest{AspectTypes: defaultAspectTypes})
	if err != nil {
		log.Warn(err)
		return err
	}

	for {
		msg, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			log.Warn(err)
		}

		log.Debug(msg.Event.Type.String(), msg.Event.UE.String())
		// processEvent(); (TODO)
	}

	return nil
}

// list all UEs
func (m *Manager) listUEs(ctx context.Context) {
	log.Info("Starting UEs listing")
	// get UEs stream
	stream, err := m.ueClient.ListUEs(ctx, &uenib.ListUERequest{AspectTypes: defaultAspectTypes})
	if err != nil {
		log.Warn(err)
		return
	}

	// Number of UEs
	num_ues := 0

	for {
		response, err := stream.Recv()
		if err == io.EOF {
			break
		}
		if err != nil {
			log.Warn(err)
		}

		// increment num of UEs
		num_ues++

		// print UE num and UE ID
		log.Debugf("UE-%d with ID %v connected", num_ues, response.UE.ID)

		// get UE aspectTypes
		aspects := m.getUEAspects(ctx, response.UE.ID)
		log.Debugf("Available aspects of UE-%d: %s", num_ues, aspects)
	}

	log.Infof("Total connected UEs: %d", num_ues)
}

// func (m *Manager) getUes() (uenib.UE, error) { // TODO
// }

// getUE gets UE aspects
func (m *Manager) getUEAspects(ctx context.Context, ueID uenib.ID) []string {
	response, err := m.ueClient.GetUE(ctx, &uenib.GetUERequest{ID: ueID})
	if err != nil {
		log.Warn(err)
		return nil
	}

	var aspects []string

	for k := range response.UE.GetAspects() {
		aspects = append(aspects, k)
	}

	return aspects
}

func (m *Manager) createUEAspect(ctx context.Context) {
}

// TODO: it is not necessary for now
func (m *Manager) updateUEAspects(ctx context.Context, ue uenib.UE) {
	log.Debug("Updating UE aspects")

	// uncomment me to set aspect to existing UE
	// ue.SetAspect(&uenib.CellInfo{ServingCell: &uenib.CellConnection{
	// 	ID:             "e2:4/e00/2/64/e0000",
	// 	SignalStrength: 11.0,
	// }})

	_, err := m.ueClient.UpdateUE(ctx, &uenib.UpdateUERequest{
		UE: ue,
	})

	if err != nil {
		log.Warn(err)
	}
}

// ConnectUeNibServiceHost connects to UE NIB service
func connectUeNibServiceHost(UeNibServiceAddress string) (*grpc.ClientConn, error) {
	tlsConfig, err := creds.GetClientCredentials()
	if err != nil {
		return nil, err
	}
	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(credentials.NewTLS(tlsConfig)),
	}
	opts = append(opts, grpc.WithUnaryInterceptor(retry.RetryingUnaryClientInterceptor()))
	return grpc.DialContext(context.Background(), UeNibServiceAddress, opts...)
}
