package broker

import (
	"container/list"
	"context"
	"io"
	"sync"

	e2api "github.com/onosproject/onos-api/go/onos/e2t/e2/v1beta1"
	e2client "github.com/onosproject/onos-ric-sdk-go/pkg/e2/v1beta1"
)

// **************To broker.go**************

// Broker is a subscription stream broker
// The Broker is responsible for managing Streams for propagating indications from the southbound API
// to the northbound API.
type Broker interface {
	io.Closer

	// OpenReader opens a subscription Stream
	// If a stream already exists for the subscription, the existing stream will be returned.
	// If no stream exists, a new stream will be allocated with a unique StreamID.
	OpenReader(ctx context.Context, node e2client.Node,
		subName string, id e2api.ChannelID, subSpec e2api.SubscriptionSpec) (StreamReader, error)

	// CloseStream closes a subscription Stream
	// The associated Stream will be closed gracefully: the reader will continue receiving pending indications
	// until the buffer is empty.
	CloseStream(ctx context.Context, id e2api.ChannelID) (StreamReader, error)

	// GetWriter gets a write stream by its StreamID
	// If no Stream exists for the given StreamID, a NotFound error will be returned.
	GetWriter(id StreamID) (StreamWriter, error)

	// ChannelIDs get all of subscription channel IDs
	ChannelIDs() []e2api.ChannelID
}

type streamBroker struct {
	subs     map[e2api.ChannelID]Stream
	streams  map[StreamID]Stream
	streamID StreamID
	mu       sync.RWMutex
}

// **************To stream.go**************

// max stream buffer
const bufferMaxSize = 10000

// Stream is a read/write stream
type Stream interface {
	StreamIO
	StreamReader
	StreamWriter
}

// StreamReader defines methods for reading indications from a Stream
type StreamReader interface {
	StreamIO

	// Recv reads an indication from the stream
	// This method is thread-safe. If multiple goroutines are receiving from the stream, indications will be
	// distributed randomly between them. If no indications are available, the goroutine will be blocked until
	// an indication is received or the Context is canceled. If the context is canceled, a context.Canceled error
	// will be returned. If the stream has been closed, an io.EOF error will be returned.
	Recv(context.Context) (e2api.Indication, error)
}

// StreamWriter is a write stream
type StreamWriter interface {
	StreamIO

	// Send sends an indication on the stream
	// The Send method is non-blocking. If no StreamReader is immediately available to consume the indication
	// it will be placed in a bounded memory buffer. If the buffer is full, an Unavailable error will be returned.
	// This method is thread-safe.
	Send(indication e2api.Indication) error
}

// StreamIO is a base interface for Stream information
type StreamIO interface {
	io.Closer
	ChannelID() e2api.ChannelID
	StreamID() StreamID
	SubscriptionName() string
	Subscription() e2api.SubscriptionSpec
	Node() e2client.Node
}

// StreamID is a stream identifier
type StreamID int

// BufferedIO data struct
type bufferedIO struct {
	subSepc   e2api.SubscriptionSpec
	streamID  StreamID
	channelID e2api.ChannelID
	node      e2client.Node
	subName   string
}

// bufferedReader is a buffer reader to channel
type bufferedReader struct {
	ch <-chan e2api.Indication
}

// bufferedReader is a buffer writer to channel
type bufferedWriter struct {
	ch     chan<- e2api.Indication
	buffer *list.List
	cond   *sync.Cond
	closed bool
}

// Stream's buffer that implement BufferedIO
type bufferedStream struct {
	*bufferedIO
	*bufferedReader
	*bufferedWriter
}
