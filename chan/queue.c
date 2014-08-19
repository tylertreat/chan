#define _GNU_SOURCE

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "queue.h"


// Returns 0 if the queue is not at capacity or is unbounded. Returns 1
// otherwise.
static int queue_at_capacity(queue_t* queue);

// Allocates and returns a new queue. The capacity specifies the maximum
// number of items that can be in the queue at one time. A capacity of -1
// means the queue is unbounded. Returns NULL if initialization failed.
queue_t* queue_init(int capacity)
{
    queue_t* queue = (queue_t*) malloc(sizeof(queue_t));
    if (!queue)
    {
        perror("Failed to allocate queue");
        return NULL;
    }

    queue->size = 0;
    queue->capacity = capacity;
    return queue;
}

// Releases the queue resources.
void queue_dispose(queue_t* queue)
{
    while (queue->size > 0)
    {
        queue_remove(&queue);
    }
    free(queue);
}

// Enqueues an item in the queue. Returns 0 is the add succeeded or -1 if it
// failed.
int queue_add(queue_t** queue, void* value)
{
    if (queue_at_capacity(*queue))
    {
        return -1;
    }

    _node_t* item = (_node_t*) malloc(sizeof(_node_t));
    if (!item)
    {
        return -1;
    }

    item->value = value;

    if ((*queue)->size == 0)
    {
        // Channel is empty.
        (*queue)->head = item;
        item->next = item;
        item->prev = item;
    }
    else if ((*queue)->size == 1)
    {
        // Single item in queue.
        item->next = (*queue)->head;
        item->prev = (*queue)->head;
        (*queue)->head->next = item;
        (*queue)->head->prev = item;
    }
    else
    {
        // More than 1 item in queue.
        (*queue)->head->next->prev = item;
        item->next = (*queue)->head->next;
        item->prev = (*queue)->head;
        (*queue)->head->next = item;
    }

    (*queue)->size++;
    return 0;
}

// Dequeues an item from the head of the queue. Returns NULL if the queue is
// empty.
void* queue_remove(queue_t** queue)
{
    _node_t* curr;

    if ((*queue)->size == 0)
    {
        // Empty queue.
        curr = NULL;
    }
    else if ((*queue)->size == 1)
    {
        // Single item in queue.
        curr = (*queue)->head;
        (*queue)->head = NULL;
        (*queue)->size = 0;
    }
    else
    {
        // Read the head of the queue.
        curr = (*queue)->head;
        (*queue)->head = curr->prev;
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
        (*queue)->size--;
    }

    void* value = NULL;

    if (curr)
    {
        value = curr->value;
        free(curr);
    }

    return value;
}

// Returns, but does not remove, the head of the queue. Returns NULL if the
// queue is empty.
void* queue_peek(queue_t* queue)
{
    if (queue->size == 0)
    {
        // Empty queue.
        return NULL;
    }

    // Read the head of the queue.
    return queue->head->value;
}

// Returns 0 if the queue is not at capacity or is unbounded. Returns 1
// otherwise.
static int queue_at_capacity(queue_t* queue)
{
    int at_capacity;

    if (queue->capacity < 0)
    {
        at_capacity = 0;
    }
    else if (queue->size >= queue->capacity)
    {
        at_capacity = 1;
    }
    else
    {
        at_capacity = 0;
    }

    return at_capacity;
}
