#define _GNU_SOURCE

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "chan.h"
#include "queue.h"
#include "sync/pipe_sem.h"
#include "sync/reentrant_lock.h"


static int buffered_chan_init(chan_t* chan, int capacity);
static int buffered_chan_send(chan_t* chan, chan_msg_t* msg);
static int buffered_chan_recv(chan_t* chan, chan_msg_t* msg);

static int unbuffered_chan_init(chan_t* chan);
static int unbuffered_chan_send(chan_t* chan, chan_msg_t* msg);
static int unbuffered_chan_recv(chan_t* chan, chan_msg_t* msg);

// Allocates and returns a new channel. The capacity specifies whether the
// channel should be buffered or not. A capacity of 0 will create an unbuffered
// channel. Returns NULL if initialization failed or the capacity is less than
// 0.
chan_t* chan_init(int capacity)
{
    if (capacity < 0)
    {
        perror("Capacity cannot be less than 0");
        return NULL;
    }

    chan_t* chan = (chan_t*) malloc(sizeof(chan_t));
    if (!chan)
    {
        perror("Failed to allocate channel");
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

    reentrant_lock_t* lock = reentrant_lock_init();
    if (!lock)
    {
        queue_dispose(queue);
        return -1;
    }

    if (unbuffered_chan_init(chan) != 0)
    {
        queue_dispose(queue);
        reentrant_lock_dispose(lock);
        return -1;
    }
    
    chan->queue = queue;
    chan->lock = lock;
    return 0;
}

static int unbuffered_chan_init(chan_t* chan)
{
    mutex_t* w_mu = mutex_init();
    if (!w_mu)
    {
        perror("Failed to initialize write mutex");
        return -1;
    }

    mutex_t* r_mu = mutex_init();
    if (!r_mu)
    {
        perror("Failed to initialize read mutex");
        mutex_dispose(w_mu);
        return -1;
    }

    pthread_mutex_t* mu = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!mu)
    {
        perror("Failed to allocate monitor mutex");
        mutex_dispose(w_mu);
        mutex_dispose(r_mu);
        return -1;
    }

    if (pthread_mutex_init(mu, NULL) != 0)
    {
        perror("Failed to initialize monitor mutex");
        free(mu);
        mutex_dispose(w_mu);
        mutex_dispose(r_mu);
        return -1;
    }

    pthread_cond_t* cond = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
    if (!cond)
    {
        perror("Failed to allocate monitor condition");
        pthread_mutex_destroy(mu);
        mutex_dispose(w_mu);
        mutex_dispose(r_mu);
        return -1;
    }

    if (pthread_cond_init(cond, NULL) != 0)
    {
        perror("Failed to initialize monitor condition");
        free(cond);
        pthread_mutex_destroy(mu);
        mutex_dispose(w_mu);
        mutex_dispose(r_mu);
        return -1;
    }

    if (pipe(chan->rw_pipe) != 0)
    {
        perror("Failed to initialize read-write pipe");
        free(cond);
        pthread_mutex_destroy(mu);
        mutex_dispose(w_mu);
        mutex_dispose(r_mu);
        return -1;
    }

    chan->m_mu = mu;
    chan->m_cond = cond;
    chan->w_mu = w_mu;
    chan->r_mu = r_mu;
    chan->readers = 0;
    return 0;
}

// Releases the channel resources.
void chan_dispose(chan_t* chan)
{
    if (chan->buffered)
    {
        queue_dispose(chan->queue);
        reentrant_lock_dispose(chan->lock);
    }
    else
    {
        mutex_dispose(chan->w_mu);
        mutex_dispose(chan->r_mu);
        close(chan->rw_pipe[0]);
        close(chan->rw_pipe[1]);
    }

    pthread_mutex_destroy(chan->m_mu);
    pthread_cond_destroy(chan->m_cond);
    free(chan);
}

// Sends a value into the channel. If the channel is unbuffered, this will
// block until a receiver receives the value. If the channel is buffered and at
// capacity, this will block until a receiver receives a value. Returns 0 if
// the send succeeded or -1 if it failed.
int chan_send(chan_t* chan, chan_msg_t* msg)
{
    if (chan->buffered)
    {
        return buffered_chan_send(chan, msg);
    }
    return unbuffered_chan_send(chan, msg);
}

static int buffered_chan_send(chan_t* chan, chan_msg_t* msg)
{
    reentrant_lock(chan->lock);
    while (chan->queue->size == chan->queue->capacity)
    {
        // Block until something is removed.
        reentrant_unlock(chan->lock);
        pthread_cond_wait(chan->m_cond, chan->m_mu);
        reentrant_lock(chan->lock);
    }

    int success = queue_add(&chan->queue, msg);

    if (chan->queue->size == 1)
    {
        // If the buffer was previously empty, notify.
        pthread_cond_signal(chan->m_cond);
    }

    reentrant_unlock(chan->lock);
    return success;
}

static int unbuffered_chan_send(chan_t* chan, chan_msg_t* msg)
{
    mutex_lock(chan->w_mu);
    while (chan->readers == 0)
    {
        // Block until there is a reader.
        pthread_cond_wait(chan->m_cond, chan->m_mu);
    }

    write(chan->rw_pipe[1], &msg, sizeof(chan_msg_t*));

    mutex_unlock(chan->w_mu);
    return 0;
}

// Receives a value from the channel. This will block until there is data to
// receive. Returns 0 if the receive succeeded or -1 if it failed.
int chan_recv(chan_t* chan, chan_msg_t* msg)
{
    return chan->buffered ?
        buffered_chan_recv(chan, msg) :
        unbuffered_chan_recv(chan, msg);
}

static int buffered_chan_recv(chan_t* chan, chan_msg_t* msg)
{
    reentrant_lock(chan->lock);
    while (chan->queue->size == 0)
    {
        // Block until something is added.
        reentrant_unlock(chan->lock);
        pthread_cond_wait(chan->m_cond, chan->m_mu);
        reentrant_lock(chan->lock);
    }

    chan_msg_t* ptr = queue_remove(&chan->queue);
    msg->data = ptr->data;
    free(ptr);

    if (chan->queue->size == chan->queue->capacity - 1)
    {
        // If the buffer was previously full, notify.
        pthread_cond_signal(chan->m_cond);
    }

    reentrant_unlock(chan->lock);
    return 0;
}

static int unbuffered_chan_recv(chan_t* chan, chan_msg_t* msg)
{
    mutex_lock(chan->r_mu);
    chan->readers++;
    pthread_cond_signal(chan->m_cond);

    int success = 0;
    void* msg_ptr = malloc(sizeof(chan_msg_t*));
    if (!msg_ptr)
    {
        success = -1;
    }
    else
    {
        read(chan->rw_pipe[0], msg_ptr, sizeof(chan_msg_t*));
        chan_msg_t* ptr = (chan_msg_t*) *(long*) msg_ptr;
        msg->data = ptr->data;
        free(ptr);
    }

    chan->readers--;
    mutex_unlock(chan->r_mu);
    return success;
}

// Returns the number of items in the channel buffer. If the channel is
// unbuffered, this will return 0.
int chan_size(chan_t* chan)
{
    int size = 0;
    if (chan->buffered)
    {
        reentrant_lock(chan->lock);
        size = chan->queue->size;
        reentrant_unlock(chan->lock);
    }
    return size;
}
