#define _GNU_SOURCE
#undef __STRICT_ANSI__

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef _WIN32
#include <stdio.h>
#include <fcntl.h>

int pipe(int pipefd[2])
{
    return _pipe(pipefd, BUFSIZ, O_BINARY);
}
#endif

#include "blocking_pipe.h"


// Allocates and returns a new blocking pipe. Sets errno and returns NULL if
// initialization failed.
blocking_pipe_t* blocking_pipe_init()
{
    blocking_pipe_t* p = malloc(sizeof(blocking_pipe_t));
    if (!p)
    {
        errno = ENOMEM;
        return NULL;
    }

    if (pthread_mutex_init(&p->mu, NULL) != 0)
    {
        free(p);
        return NULL;
    }

    if (pthread_cond_init(&p->cond, NULL) != 0)
    {
        pthread_mutex_destroy(&p->mu);
        free(p);
        return NULL;
    }


    if (pipe(p->rw_pipe) != 0)
    {
        pthread_mutex_destroy(&p->mu);
        pthread_cond_destroy(&p->cond);
        free(p);
        return NULL;
    }

    p->reader = 0;
    p->sender = 0;
    return p;
}

// Releases the pipe resources.
void blocking_pipe_dispose(blocking_pipe_t* pipe)
{
    pthread_mutex_destroy(&pipe->mu);
    pthread_cond_destroy(&pipe->cond);
    close(pipe->rw_pipe[0]);
    close(pipe->rw_pipe[1]);
    free(pipe);
}

// Reads from the pipe and sets the pointer to the read data. This will block
// until a writer is ready. Returns 0 if the read was successful, -1 if not.
int blocking_pipe_read(blocking_pipe_t* pipe, void** data)
{
    pthread_mutex_lock(&pipe->mu);
    pipe->reader = 1;
    pthread_cond_signal(&pipe->cond);
    pthread_mutex_unlock(&pipe->mu);

    return read(pipe->rw_pipe[0], data, sizeof(void*)) > 0 ? 0 : -1;
}

// Writes the given pointer to the pipe. This will block until a reader is
// ready. Returns 0 if the write was successful, -1 if not.
int blocking_pipe_write(blocking_pipe_t* pipe, void* data)
{
    pthread_mutex_lock(&pipe->mu);
    pipe->sender = 1;
    while (pipe->reader == 0)
    {
        pthread_cond_wait(&pipe->cond, &pipe->mu);
    }
    pipe->reader = 0;
    pipe->sender = 0;
    int success = write(pipe->rw_pipe[1], &data, sizeof(void*)) > 0 ? 0 : -1;
    pthread_mutex_unlock(&pipe->mu);
    return success;
}
