package broker

import (
	"context"

	e2api "github.com/onosproject/onos-api/go/onos/e2t/e2/v1beta1"
	"github.com/onosproject/onos-lib-go/pkg/errors"
	"github.com/onosproject/onos-lib-go/pkg/logging"
	e2client "github.com/onosproject/onos-ric-sdk-go/pkg/e2/v1beta1"
)

// log initialize
var log = logging.GetLogger("iqos-xapp", "broker")

// Is this declaration necessary??
var _ Broker = &streamBroker{}

// NewBroker creates a new subscription stream broker
func NewBroker() Broker {
	return &streamBroker{
		subs:    make(map[e2api.ChannelID]Stream), // maps of streams by subscription ID
		streams: make(map[StreamID]Stream),        // maps of streams by stream ID
	}
}

// ChannelIDs return the list of subscription channels IDs openeds
func (b *streamBroker) ChannelIDs() []e2api.ChannelID {
	b.mu.Lock()                                        // lock mutex
	defer b.mu.Unlock()                                // unlock mutex
	channelIDs := make([]e2api.ChannelID, len(b.subs)) // creates a list with the size of the subscription channel
	for channelID := range b.subs {
		channelIDs = append(channelIDs, channelID)
	}
	return channelIDs
}

// OpenReader creates a new reader to subscription channel
func (b *streamBroker) OpenReader(_ context.Context, node e2client.Node, subName string, channelID e2api.ChannelID, subSpec e2api.SubscriptionSpec) (StreamReader, error) {
	b.mu.RLock()
	stream, ok := b.subs[channelID] // search stream by channelID
	b.mu.RUnlock()
	if ok {
		return stream, nil // stream already exist
	}

	b.mu.Lock()
	defer b.mu.Unlock()

	b.streamID++ // increment streamID (i + 1 | i start in 0)
	streamID := b.streamID
	stream = newBufferedStream(node, subName, streamID, channelID, subSpec) // creates a new stream to subscription
	b.subs[channelID] = stream                                              // stores stream on the subscriptions map
	b.streams[streamID] = stream                                            // stores stream on the streams map
	log.Infof("Opened new stream %d for subscription channel '%s'", streamID, channelID)
	return stream, nil
}

// CloseStream closes a subscription Stream by channelID
func (b *streamBroker) CloseStream(ctx context.Context, id e2api.ChannelID) (StreamReader, error) {
	b.mu.Lock()
	defer b.mu.Unlock()
	stream, ok := b.subs[id] // search stream by channelID
	if !ok {
		return nil, errors.NewNotFound("subscription '%s' not found", id)
	}

	log.Debugf("Deleting Subscription: %s", stream.SubscriptionName())
	err := stream.Node().Unsubscribe(ctx, stream.SubscriptionName()) // node's unsubscribe
	if err != nil {
		return nil, err
	}

	delete(b.subs, stream.ChannelID())   // delete stream from subscription map
	delete(b.streams, stream.StreamID()) // delete stream from stream map

	log.Infof("Closed stream %d for subscription '%s'", stream.StreamID(), id)
	return stream, stream.Close() // closes stream
}

// GetWriter gets the stream writer by streamID
func (b *streamBroker) GetWriter(id StreamID) (StreamWriter, error) {
	b.mu.RLock()
	defer b.mu.RUnlock()
	stream, ok := b.streams[id] // get stream on map by its ID
	if !ok {
		return nil, errors.NewNotFound("stream %d not found", id)
	}
	return stream, nil
}

// Used by the CloseStream to close stream :)
func (b *streamBroker) Close() error {
	b.mu.Lock()
	defer b.mu.Unlock()
	var err error
	for _, stream := range b.streams {
		if e := stream.Close(); e != nil {
			err = e
		}
	}
	return err
}
