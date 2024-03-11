package broker

import (
	"container/list"
	"context"
	"io"
	"sync"

	"github.com/atomix/atomix/api/errors"
	e2api "github.com/onosproject/onos-api/go/onos/e2t/e2/v1beta1"
	e2client "github.com/onosproject/onos-ric-sdk-go/pkg/e2/v1beta1"
)

// Is this declaration necessary??
var _ Stream = &bufferedStream{}

// creates a new buffered stream
func newBufferedStream(node e2client.Node, subName string, streamID StreamID, channelID e2api.ChannelID, subSpec e2api.SubscriptionSpec) Stream {
	ch := make(chan e2api.Indication)
	return &bufferedStream{
		bufferedIO: &bufferedIO{
			streamID:  streamID,
			channelID: channelID,
			subSepc:   subSpec,
			node:      node,
			subName:   subName,
		},
		bufferedReader: newBufferedReader(ch),
		bufferedWriter: newBufferedWriter(ch),
	}
}

// creates a new buffer reader
func newBufferedReader(ch <-chan e2api.Indication) *bufferedReader {
	return &bufferedReader{
		ch: ch,
	}
}

// creates a new buffer writer
func newBufferedWriter(ch chan<- e2api.Indication) *bufferedWriter {
	writer := &bufferedWriter{
		ch:     ch,
		buffer: list.New(),
		cond:   sync.NewCond(&sync.Mutex{}),
	}
	writer.open()
	return writer
}

// open starts the goroutine propagating indications from the writer to the reader
func (s *bufferedWriter) open() {
	go s.drain()
}

// drain dequeues indications and writes them to the read channel
func (s *bufferedWriter) drain() {
	for {
		ind, ok := s.next()
		if !ok {
			close(s.ch)
			break
		}
		s.ch <- ind
	}
}

// next reads the next indication from the buffer or blocks until one becomes available
func (s *bufferedWriter) next() (e2api.Indication, bool) {
	s.cond.L.Lock()
	defer s.cond.L.Unlock()
	for s.buffer.Len() == 0 {
		if s.closed {
			return e2api.Indication{}, false
		}
		s.cond.Wait()
	}
	result := s.buffer.Front().Value.(e2api.Indication)
	s.buffer.Remove(s.buffer.Front())
	return result, true
}

// Send appends the indication to the buffer and notifies the reader
func (s *bufferedWriter) Send(ind e2api.Indication) error {
	s.cond.L.Lock()
	defer s.cond.L.Unlock()
	if s.closed {
		return io.EOF
	}
	if s.buffer.Len() == bufferMaxSize {
		return errors.NewUnavailable("cannot append indication to stream: maximum buffer size has been reached")
	}
	s.buffer.PushBack(ind)
	s.cond.Signal()
	return nil
}

// Recv receive the indication from the buffer
func (s *bufferedReader) Recv(ctx context.Context) (e2api.Indication, error) {
	select {
	case ind, ok := <-s.ch:
		if !ok {
			return e2api.Indication{}, io.EOF
		}
		return ind, nil
	case <-ctx.Done():
		return e2api.Indication{}, ctx.Err()
	}
}

// close the buffer for writing
func (s *bufferedWriter) Close() error {
	s.cond.L.Lock()
	defer s.cond.L.Unlock()
	s.closed = true
	return nil
}

// return Subscription Name buffer info
func (s *bufferedIO) SubscriptionName() string {
	return s.subName
}

// return Subscription Spec buffer info
func (s *bufferedIO) Subscription() e2api.SubscriptionSpec {
	return s.subSepc
}

// return channelID buffer info
func (s *bufferedIO) ChannelID() e2api.ChannelID {
	return s.channelID
}

// return e2Node buffer info
func (s *bufferedIO) Node() e2client.Node {
	return s.node
}

// return StreamID buffer info
func (s *bufferedIO) StreamID() StreamID {
	return s.streamID
}
