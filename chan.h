#ifndef chan_h
#define chan_h

#include "sync/reentrant_lock.h"


// Defines a single item in the channel.
typedef struct _node_t
{
    struct _node_t* next;
    struct _node_t* prev;
    void*           value;
} _node_t;

// Defines a threadsafe, circular, doubly-linked list which acts as a queue.
typedef struct chan_t
{
    _node_t*          head;
    reentrant_lock_t* lock;
    int               size;
    int               capacity;
} chan_t;

// Allocates and returns a new channel. The capacity specifies the maximum
// number of items that can be in the channel at one time. A capacity of -1
// means the channel is unbounded. Returns NULL if initialization failed.
chan_t* chan_init(int capacity);

// Releases the channel resources.
void chan_dispose(chan_t* chan);

// Enqueues an item in the channel. Returns 0 is the send failed or 1 if it
// succeeded.
int chan_send(chan_t** chan, void* value);

// Dequeues an item from the channel. Returns NULL if the channel is empty.
void* chan_recv(chan_t** chan);

// Returns the number of items currently in the channel.
int chan_size(chan_t* chan);

#endif
