#define _GNU_SOURCE

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "blocking_pipe.h"
#include "chan.h"
#include "queue.h"


static int buffered_chan_init(chan_t* chan, int capacity);
static int buffered_chan_send(chan_t* chan, void* data);
static int buffered_chan_recv(chan_t* chan, void** data);

static int unbuffered_chan_init(chan_t* chan);
static int unbuffered_chan_send(chan_t* chan, void* data);
static int unbuffered_chan_recv(chan_t* chan, void** data);

// Allocates and returns a new channel. The capacity specifies whether the
// channel should be buffered or not. A capacity of 0 will create an unbuffered
// channel. Sets errno and returns NULL if initialization failed or the
// capacity is less than 0.
chan_t* chan_init(int capacity)
{
    if (capacity < 0)
    {
        errno = EINVAL;
        return NULL;
    }

    chan_t* chan = (chan_t*) malloc(sizeof(chan_t));
    if (!chan)
    {
        errno = ENOMEM;
        return NULL;
    }

    if (capacity > 0)
    {
        if (buffered_chan_init(chan, capacity) != 0)
        {
            free(chan);
            return NULL;
        }
        chan->buffered = 1;
    }
    else
    {
        if (unbuffered_chan_init(chan) != 0)
        {
            free(chan);
            return NULL;
        }
        chan->buffered = 0;
    }
    
    return chan;
}

static int buffered_chan_init(chan_t* chan, int capacity)
{
    queue_t* queue = queue_init(capacity);
    if (!queue)
    {
        return -1;
    }

    if (unbuffered_chan_init(chan) != 0)
    {
        queue_dispose(queue);
        return -1;
    }
    
    chan->queue = queue;
    return 0;
}

static int unbuffered_chan_init(chan_t* chan)
{
    pthread_mutex_t* w_mu = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!w_mu)
    {
        errno = ENOMEM;
        return -1;
    }

    if (pthread_mutex_init(w_mu, NULL) != 0)
    {
        free(w_mu);
    }

    pthread_mutex_t* r_mu = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!r_mu)
    {
        errno = ENOMEM;
        pthread_mutex_destroy(w_mu);
        return -1;
    }

    if (pthread_mutex_init(r_mu, NULL) != 0)
    {
        free(r_mu);
        pthread_mutex_destroy(w_mu);
        return -1;
    }

    pthread_mutex_t* mu = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!mu)
    {
        errno = ENOMEM;
        pthread_mutex_destroy(w_mu);
        pthread_mutex_destroy(r_mu);
        return -1;
    }

    if (pthread_mutex_init(mu, NULL) != 0)
    {
        free(mu);
        pthread_mutex_destroy(w_mu);
        pthread_mutex_destroy(r_mu);
        return -1;
    }

    pthread_cond_t* cond = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
    if (!cond)
    {
        errno = ENOMEM;
        pthread_mutex_destroy(mu);
        pthread_mutex_destroy(w_mu);
        pthread_mutex_destroy(r_mu);
        return -1;
    }

    if (pthread_cond_init(cond, NULL) != 0)
    {
        free(cond);
        pthread_mutex_destroy(mu);
        pthread_mutex_destroy(w_mu);
        pthread_mutex_destroy(r_mu);
        return -1;
    }

    blocking_pipe_t* pipe = blocking_pipe_init();
    if (pipe == NULL)
    {
        free(cond);
        pthread_mutex_destroy(mu);
        pthread_mutex_destroy(w_mu);
        pthread_mutex_destroy(r_mu);
        return -1;
    }

    chan->m_mu = mu;
    chan->m_cond = cond;
    chan->w_mu = w_mu;
    chan->r_mu = r_mu;
    chan->readers = 0;
    chan->closed = 0;
    chan->pipe = pipe;
    return 0;
}

// Releases the channel resources.
void chan_dispose(chan_t* chan)
{
    if (chan->buffered)
    {
        queue_dispose(chan->queue);
    }
    else
    {
        pthread_mutex_destroy(chan->w_mu);
        pthread_mutex_destroy(chan->r_mu);
        blocking_pipe_dispose(chan->pipe);
    }

    pthread_mutex_destroy(chan->m_mu);
    pthread_cond_destroy(chan->m_cond);
    free(chan);
}

// Once a channel is closed, data cannot be sent into it. If the channel is
// buffered, data can be read from it until it is empty, after which reads will
// return an error code. Reading from a closed channel that is unbuffered will
// return an error code. Closing a channel does not release its resources. This
// must be done with a call to chan_dispose. Returns 0 if the channel was
// successfully closed, -1 otherwise. If -1 is returned, errno will be set.
int chan_close(chan_t* chan)
{
    int success = 0;
    pthread_mutex_lock(chan->m_mu);
    if (chan->closed)
    {
        // Channel already closed.
        success = -1;
        errno = EPIPE;
    }
    else
    {
        // Otherwise close it.
        chan->closed = 1;
        if (!chan->buffered)
        {
            // Closing pipe will unblock any potential waiting reader.
            close(chan->pipe->rw_pipe[0]);
            close(chan->pipe->rw_pipe[1]);
        }
        pthread_cond_signal(chan->m_cond);
    }
    pthread_mutex_unlock(chan->m_mu);
    return success;
}

// Returns 0 if the channel is open and 1 if it is closed.
int chan_is_closed(chan_t* chan)
{
    pthread_mutex_lock(chan->m_mu);
    int closed = chan->closed;
    pthread_mutex_unlock(chan->m_mu);
    return closed;
}

// Sends a value into the channel. If the channel is unbuffered, this will
// block until a receiver receives the value. If the channel is buffered and at
// capacity, this will block until a receiver receives a value. Returns 0 if
// the send succeeded or -1 if it failed. If -1 is returned, errno will be set.
int chan_send(chan_t* chan, void* data)
{
    if (chan_is_closed(chan))
    {
        // Cannot send on closed channel.
        errno = EPIPE;
        return -1;
    }

    if (chan->buffered)
    {
        return buffered_chan_send(chan, data);
    }
    return unbuffered_chan_send(chan, data);
}

// Receives a value from the channel. This will block until there is data to
// receive. Returns 0 if the receive succeeded or -1 if it failed. If -1 is
// returned, errno will be set.
int chan_recv(chan_t* chan, void** data)
{
    return chan->buffered ?
        buffered_chan_recv(chan, data) :
        unbuffered_chan_recv(chan, data);
}

static int buffered_chan_send(chan_t* chan, void* data)
{
    pthread_mutex_lock(chan->m_mu);
    while (chan->queue->size == chan->queue->capacity)
    {
        // Block until something is removed.
        pthread_cond_wait(chan->m_cond, chan->m_mu);
    }

    int success = queue_add(&chan->queue, data);

    if (chan->queue->size == 1)
    {
        // If the buffer was previously empty, notify.
        pthread_cond_signal(chan->m_cond);
    }

    pthread_mutex_unlock(chan->m_mu);
    return success;
}

static int buffered_chan_recv(chan_t* chan, void** data)
{
    pthread_mutex_lock(chan->m_mu);
    while (chan->queue->size == 0)
    {
        if (chan->closed)
        {
            pthread_mutex_unlock(chan->m_mu);
            errno = EPIPE;
            return -1;
        }

        // Block until something is added.
        pthread_cond_wait(chan->m_cond, chan->m_mu);
    }

    *data = queue_remove(&chan->queue);

    if (chan->queue->size == chan->queue->capacity - 1)
    {
        // If the buffer was previously full, notify.
        pthread_cond_signal(chan->m_cond);
    }

    pthread_mutex_unlock(chan->m_mu);
    return 0;
}

static int unbuffered_chan_send(chan_t* chan, void* data)
{
    pthread_mutex_lock(chan->w_mu);
    int success = blocking_pipe_write(chan->pipe, data);
    pthread_mutex_unlock(chan->w_mu);
    return success;
}

static int unbuffered_chan_recv(chan_t* chan, void** data)
{
    pthread_mutex_lock(chan->r_mu);
    if (chan_is_closed(chan))
    {
        errno = EPIPE;
        return -1;
    }

    int success = blocking_pipe_read(chan->pipe, data);

    pthread_mutex_unlock(chan->r_mu);
    return success;
}

// Returns the number of items in the channel buffer. If the channel is
// unbuffered, this will return 0.
int chan_size(chan_t* chan)
{
    int size = 0;
    if (chan->buffered)
    {
        pthread_mutex_lock(chan->m_mu);
        size = chan->queue->size;
        pthread_mutex_unlock(chan->m_mu);
    }
    return size;
}
