package clientv

import (
	"context"
	"crypto/tls"
	"fmt"
	"sync"

	rsmapi "github.com/onosproject/onos-api/go/onos/rsm"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials"
)

func NewConnection() (*grpc.ClientConn, error) {
	address := "localhost:5150"
	certPath := TLSCrtPath
	keyPath := TLSKeyPath
	var opts []grpc.DialOption
	cert, err := tls.LoadX509KeyPair(certPath, keyPath)
	if err != nil {
		return nil, err
	}
	opts = []grpc.DialOption{
		grpc.WithTransportCredentials(credentials.NewTLS(&tls.Config{
			Certificates:       []tls.Certificate{cert},
			InsecureSkipVerify: true,
		})),
	}

	conn, err := grpc.Dial(address, opts...)
	if err != nil {
		return nil, err
	}
	return conn, nil
}

func CmdCreateSlice(e2NodeID string, sliceID uint64, schedulerType string, weight uint64, sliceType string) error {

	conn, err := NewConnection()
	if err != nil {
		return err
	}
	defer conn.Close()
	client := rsmapi.NewRsmClient(conn)

	errCh := make(chan error)
	succCh := make(chan struct{})
	wg := sync.WaitGroup{}
	wg.Add(1)

	go func() {
		defer wg.Done()

		setRequest := rsmapi.CreateSliceRequest{
			E2NodeId:      e2NodeID,
			SliceId:       fmt.Sprintf("%d", sliceID),
			Weight:        fmt.Sprintf("%d", weight),
			SchedulerType: GetSliceScheduler(schedulerType),
			SliceType:     GetSliceType(sliceType),
		}
		fmt.Printf("%s\n", &setRequest)
		resp, err := client.CreateSlice(context.Background(), &setRequest)
		if err != nil {
			errCh <- err
		}
		if !resp.GetAck().GetSuccess() {
			errCh <- fmt.Errorf(resp.GetAck().GetCause())
		}
		succCh <- struct{}{}
	}()

	go func() {
		wg.Wait()     // Espere que todas as goroutines adicionadas à WaitGroup sejam concluídas
		close(succCh) // Feche o canal de sucesso quando todas as goroutines forem concluídas
	}()

	select {
	case e := <-errCh:
		return e
	case <-succCh:
	}
	return nil
}
