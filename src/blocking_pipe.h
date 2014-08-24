#ifndef blocking_pipe_h
#define blocking_pipe_h

#include <pthread.h>


// Defines a synchronized pipe in which reads and writes block until both the
// reader and writer are ready.
typedef struct blocking_pipe_t
{
    int              reader;
    int              sender;
    pthread_mutex_t* mu;
    pthread_cond_t*  cond;
    int              rw_pipe[2];
} blocking_pipe_t;

// Allocates and returns a new blocking pipe. Sets errno and returns NULL if
// initialization failed.
blocking_pipe_t* blocking_pipe_init();

// Releases the pipe resources.
void blocking_pipe_dispose(blocking_pipe_t* pipe);

// Reads from the pipe and sets the pointer to the read data. This will block
// until a writer is ready. Returns 0 if the read was successful, -1 if not.
int blocking_pipe_read(blocking_pipe_t* pipe, void** data);

// Writes the given pointer to the pipe. This will block until a reader is
// ready. Returns 0 if the write was successful, -1 if not.
int blocking_pipe_write(blocking_pipe_t* pipe, void* data);

#endif
