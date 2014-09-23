#define _GNU_SOURCE

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

#if defined(_WIN32) && !defined(ENOBUFS)
#include <winsock.h>
#define ENOBUFS WSAENOBUFS
#endif

// Returns 0 if the queue is not at capacity. Returns 1 otherwise.
static inline int queue_at_capacity(queue_t* queue)
{
    return queue->size >= queue->capacity;
}

// Allocates and returns a new queue. The capacity specifies the maximum
// number of items that can be in the queue at one time. A capacity greater
// than INT_MAX / sizeof(void*) is considered an error. Returns NULL if
// initialization failed.
queue_t* queue_init(size_t capacity)
{
    if (capacity > INT_MAX / sizeof(void*))
    {
        errno = EINVAL;
        return NULL;
    }

    queue_t* queue = (queue_t*) malloc(sizeof(queue_t));
    void**   data  = (void**) malloc(capacity * sizeof(void*));
    if (!queue || !data)
    {
        // In case of free(NULL), no operation is performed.
        free(queue);
        free(data);   
        errno = ENOMEM;
        return NULL;
    }

    queue->size = 0;
    queue->next = 0;
    queue->capacity = capacity;
    queue->data = data;
    return queue;
}

// Releases the queue resources.
void queue_dispose(queue_t* queue)
{
    free(queue->data);
    free(queue);
}

// Enqueues an item in the queue. Returns 0 is the add succeeded or -1 if it
// failed. If -1 is returned, errno will be set.
int queue_add(queue_t* queue, void* value)
{
    if (queue_at_capacity(queue))
    {
        errno = ENOBUFS;
        return -1;
    }

    int pos = queue->next + queue->size;
    if (pos >= queue->capacity)
    {
       pos -= queue->capacity;
    }

    queue->data[pos] = value;

    queue->size++;
    return 0;
}

// Dequeues an item from the head of the queue. Returns NULL if the queue is
// empty.
void* queue_remove(queue_t* queue)
{
    void* value = NULL;

    if (queue->size > 0)
    {
        value = queue->data[queue->next];
        queue->next++;
        queue->size--;
        if (queue->next >= queue->capacity)
        {
            queue->next -= queue->capacity;
        }
    }

    return value;
}

// Returns, but does not remove, the head of the queue. Returns NULL if the
// queue is empty.
void* queue_peek(queue_t* queue)
{
    return queue->size ? queue->data[queue->next] : NULL;
}
