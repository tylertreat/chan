#define _GNU_SOURCE

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "chan.h"
#include "sync/reentrant_lock.h"


// Returns 0 if the channel is not at capacity or is unbounded. Returns 1
// otherwise.
static int _chan_at_capacity(chan_t* chan);

// Allocates and returns a new channel. The capacity specifies the maximum
// number of items that can be in the channel at one time. A capacity of -1
// means the channel is unbounded. Returns NULL if initialization failed.
chan_t* chan_init(int capacity)
{
    chan_t* chan = (chan_t*) malloc(sizeof(chan_t));
    if (!chan)
    {
        perror("Failed to allocate channel");
        return NULL;
    }

    reentrant_lock_t* lock = reentrant_lock_init();
    if (!lock)
    {
        return NULL;
    }

    chan->lock = lock;
    chan->size = 0;
    chan->capacity = capacity;
    return chan;
}

// Releases the channel resources.
void chan_dispose(chan_t* chan)
{
    while (chan_size(chan) > 0)
    {
        chan_recv(&chan);
    }
    reentrant_lock_dispose(chan->lock);
    free(chan);
}

// Enqueues an item in the channel. Returns 0 is the send failed or 1 if it
// succeeded.
int chan_send(chan_t** chan, void* value)
{
    reentrant_lock((*chan)->lock);
    if (_chan_at_capacity(*chan))
    {
        reentrant_unlock((*chan)->lock);
        return 0;
    }

    _node_t* item = (_node_t*) malloc(sizeof(_node_t));
    if (!item)
    {
        reentrant_unlock((*chan)->lock);
        return 0;
    }

    item->value = value;

    if ((*chan)->size == 0)
    {
        // Channel is empty.
        (*chan)->head = item;
        item->next = item;
        item->prev = item;
    }
    else if ((*chan)->size == 1)
    {
        // Single item in channel.
        item->next = (*chan)->head;
        item->prev = (*chan)->head;
        (*chan)->head->next = item;
        (*chan)->head->prev = item;
    }
    else
    {
        // More than 1 item in channel.
        (*chan)->head->next->prev = item;
        item->next = (*chan)->head->next;
        item->prev = (*chan)->head;
        (*chan)->head->next = item;
    }

    (*chan)->size++;
    reentrant_unlock((*chan)->lock);
    return 1;
}

// Dequeues an item from the channel. Returns NULL if the channel is empty.
void* chan_recv(chan_t** chan)
{
    reentrant_lock((*chan)->lock);
    _node_t* curr;

    if ((*chan)->size == 0)
    {
        // Empty channel.
        curr = NULL;
    }
    else if ((*chan)->size == 1)
    {
        // Single item in channel.
        curr = (*chan)->head;
        (*chan)->head = NULL;
        (*chan)->size = 0;
    }
    else
    {
        // Read the head of the queue.
        curr = (*chan)->head;
        (*chan)->head = curr->prev;
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
        (*chan)->size--;
    }

    reentrant_unlock((*chan)->lock);
    void* value = NULL;

    if (curr)
    {
        value = curr->value;
        free(curr);
    }

    return value;
}

// Returns the number of items currently in the channel.
int chan_size(chan_t* chan)
{
    reentrant_lock(chan->lock);
    int size = chan->size;
    reentrant_unlock(chan->lock);
    return size;
}

// Returns 0 if the channel is not at capacity or is unbounded. Returns 1
// otherwise.
static int _chan_at_capacity(chan_t* chan)
{
    reentrant_lock(chan->lock);
    int at_capacity;

    if (chan->capacity == -1)
    {
        at_capacity = 0;
    }
    else if (chan->size >= chan->capacity)
    {
        at_capacity = 1;
    }
    else
    {
        at_capacity = 0;
    }

    reentrant_unlock(chan->lock);
    return at_capacity;
}
