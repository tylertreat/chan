#ifndef chan_h
#define chan_h

#include "queue.h"
#include "sync/mutex.h"
#include "sync/reentrant_lock.h"


// Defines a thread-safe communication pipe. Channels are either buffered or
// unbuffered. An unbuffered channel is synchronized. Receiving on either type
// of channel will block until there is data to receive. If the channel is
// unbuffered, the sender blocks until the receiver has received the value. If
// the channel is buffered, the sender only blocks until the value has been
// copied to the buffer, meaning it will block if the channel is full.
typedef struct chan_t
{
    queue_t*          queue;
    reentrant_lock_t* lock;
    pthread_mutex_t*  m_mu;
    pthread_cond_t*   m_cond;
    int               buffered;

    mutex_t*          r_mu;
    mutex_t*          w_mu;
    int               rw_pipe[2];
    int               readers;
} chan_t;

// Allocates and returns a new channel. The capacity specifies whether the
// channel should be buffered or not. A capacity of 0 will create an unbuffered
// channel. Returns NULL if initialization failed or the capacity is less than
// 0.
chan_t* chan_init(int capacity);

// Releases the channel resources.
void chan_dispose(chan_t* chan);

// Sends a value into the channel. If the channel is unbuffered, this will
// block until a receiver receives the value. If the channel is buffered and at
// capacity, this will block until a receiver receives a value. Returns 0 if
// the send succeeded or -1 if it failed.
int chan_send(chan_t* chan, void* value, size_t size);

// Receives a value from the channel. This will block until there is data to
// receive. Returns 0 if the receive succeeded or -1 if it failed.
int chan_recv(chan_t* chan, void** value, size_t size);

// Returns the number of items in the channel buffer. If the channel is
// unbuffered, this will return 0.
int chan_size(chan_t* chan);

#endif
