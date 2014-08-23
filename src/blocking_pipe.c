#define _GNU_SOURCE

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "blocking_pipe.h"


// Allocates and returns a new blocking pipe. Sets errno and returns NULL if
// initialization failed.
blocking_pipe_t* blocking_pipe_init()
{
    pthread_mutex_t* mu = malloc(sizeof(pthread_mutex_t));
    if (!mu)
    {
        errno = ENOMEM;
        return NULL;
    }

    if (pthread_mutex_init(mu, NULL) != 0)
    {
        free(mu);
        return NULL;
    }

    pthread_cond_t* cond = malloc(sizeof(pthread_cond_t));
    if (!cond)
    {
        errno = ENOMEM;
        pthread_mutex_destroy(mu);
        return NULL;
    }

    if (pthread_cond_init(cond, NULL) != 0)
    {
        free(cond);
        pthread_mutex_destroy(mu);
        return NULL;
    }

    blocking_pipe_t* p = malloc(sizeof(blocking_pipe_t));
    if (!p)
    {
        errno = ENOMEM;
        pthread_mutex_destroy(mu);
        pthread_cond_destroy(cond);
    }

    if (pipe(p->rw_pipe) != 0)
    {
        pthread_mutex_destroy(mu);
        pthread_cond_destroy(cond);
        return NULL;
    }

    p->reader = 0;
    p->mu = mu;
    p->cond = cond;
    return p;
}

// Releases the pipe resources.
void blocking_pipe_dispose(blocking_pipe_t* pipe)
{
    pthread_mutex_destroy(pipe->mu);
    pthread_cond_destroy(pipe->cond);
    close(pipe->rw_pipe[0]);
    close(pipe->rw_pipe[1]);
    free(pipe);
}

// Reads from the pipe and sets the pointer to the read data. This will block
// until a writer is ready. Returns 0 if the read was successful, -1 if not.
int blocking_pipe_read(blocking_pipe_t* pipe, void** data)
{
    pthread_mutex_lock(pipe->mu);
    pipe->reader = 1;
    int success = 0;

    void* msg_ptr = malloc(sizeof(void*));
    if (!msg_ptr)
    {
        errno = ENOMEM;
        success = -1;
    }

    pthread_cond_signal(pipe->cond);
    pthread_mutex_unlock(pipe->mu);

    if (success == 0)
    {
        success = read(pipe->rw_pipe[0], msg_ptr, sizeof(void*)) > 0 ? 0 : -1;
        if (!data)
        {
            free(msg_ptr);
        }
        else
        {
            *data = (void*) *(long*) msg_ptr;
        }
    }

    return success;
}

// Writes the given pointer to the pipe. This will block until a reader is
// ready. Returns 0 if the write was successful, -1 if not.
int blocking_pipe_write(blocking_pipe_t* pipe, void* data)
{
    pthread_mutex_lock(pipe->mu);
    while (pipe->reader == 0)
    {
        pthread_cond_wait(pipe->cond, pipe->mu);
    }
    pipe->reader = 0;
    int success = write(pipe->rw_pipe[1], &data, sizeof(void*)) > 0 ? 0 : -1;
    pthread_mutex_unlock(pipe->mu);
    return success;
}
