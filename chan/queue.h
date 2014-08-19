#ifndef queue_h
#define queue_h


// Defines a single item in the queue.
typedef struct _node_t
{
    struct _node_t* next;
    struct _node_t* prev;
    void*           value;
} _node_t;

// Defines a circular, doubly-linked list which acts as a FIFO queue.
typedef struct queue_t
{
    _node_t*          head;
    int               size;
    int               capacity;
} queue_t;

// Allocates and returns a new queue. The capacity specifies the maximum
// number of items that can be in the queue at one time. A capacity of -1
// means the queue is unbounded. Sets errno and returns NULL if initialization
// failed.
queue_t* queue_init(int capacity);

// Releases the queue resources.
void queue_dispose(queue_t* queue);

// Enqueues an item in the queue. Returns 0 if the add succeeded or -1 if it
// failed. If -1 is returned, errno will be set.
int queue_add(queue_t** queue, void* value);

// Dequeues an item from the head of the queue. Returns NULL if the queue is
// empty.
void* queue_remove(queue_t** queue);

// Returns, but does not remove, the head of the queue. Returns NULL if the
// queue is empty.
void* queue_peek(queue_t*);

#endif
