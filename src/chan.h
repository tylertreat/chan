#ifndef chan_h
#define chan_h

#include <pthread.h>

#include "blocking_pipe.h"
#include "queue.h"


// Defines a thread-safe communication pipe. Channels are either buffered or
// unbuffered. An unbuffered channel is synchronized. Receiving on either type
// of channel will block until there is data to receive. If the channel is
// unbuffered, the sender blocks until the receiver has received the value. If
// the channel is buffered, the sender only blocks until the value has been
// copied to the buffer, meaning it will block if the channel is full.
typedef struct chan_t
{
    // Buffered channel properties
    queue_t*         queue;
    
    // Unbuffered channel properties
    pthread_mutex_t* r_mu;
    pthread_mutex_t* w_mu;
    int              readers;
    blocking_pipe_t* pipe;

    // Shared properties
    pthread_mutex_t* m_mu;
    pthread_cond_t*  m_cond;
    int              buffered;
    int              closed;
} chan_t;

// Allocates and returns a new channel. The capacity specifies whether the
// channel should be buffered or not. A capacity of 0 will create an unbuffered
// channel. Sets errno and returns NULL if initialization failed or the
// capacity is less than 0.
chan_t* chan_init(int capacity);

// Releases the channel resources.
void chan_dispose(chan_t* chan);

// Once a channel is closed, data cannot be sent into it. If the channel is
// buffered, data can be read from it until it is empty, after which reads will
// return an error code. Reading from a closed channel that is unbuffered will
// return an error code. Closing a channel does not release its resources. This
// must be done with a call to chan_dispose. Returns 0 if the channel was
// successfully closed, -1 otherwise.
int chan_close(chan_t* chan);

// Returns 0 if the channel is open and 1 if it is closed.
int chan_is_closed(chan_t* chan);

// Sends a value into the channel. If the channel is unbuffered, this will
// block until a receiver receives the value. If the channel is buffered and at
// capacity, this will block until a receiver receives a value. Returns 0 if
// the send succeeded or -1 if it failed.
int chan_send(chan_t* chan, void* data);

// Receives a value from the channel. This will block until there is data to
// receive. Returns 0 if the receive succeeded or -1 if it failed.
int chan_recv(chan_t* chan, void** data);

// Returns the number of items in the channel buffer. If the channel is
// unbuffered, this will return 0.
int chan_size(chan_t* chan);

int chan_select(chan_t* recv_channels[], int recv_count, void** recv_out);

#endif
