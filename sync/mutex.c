#define _GNU_SOURCE

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>

#include "mutex.h"
#include "pipe_sem.h"

// Allocates and returns a new mutex. Returns NULL if initialization failed.
mutex_t* mutex_init()
{
    pipe_sem_t* sem = (pipe_sem_t*) malloc(sizeof(pipe_sem_t));
    if (sem == NULL)
    {
        perror("Failed to allocate semaphore");
        return NULL;
    }

    pipe_sem_init(&sem, 1);
    if (sem == NULL)
    {
        return NULL;
    }

    mutex_t* mu = (mutex_t*) malloc(sizeof(mutex_t));
    if (mu == NULL)
    {
        perror("Failed to allocate mutex");
        pipe_sem_dispose(*sem);
    }
    
    mu->sem = sem;
    return mu;
}

// Releases the mutex resources.
void mutex_dispose(mutex_t* mu)
{
    pipe_sem_dispose(*mu->sem);
    free(mu);
}

// Acquires a lock on the mutex.
void mutex_lock(mutex_t* mu)
{
    pipe_sem_wait(*mu->sem);
}

// Releases the lock on the mutex.
void mutex_unlock(mutex_t* mu)
{
    pipe_sem_signal(*mu->sem);
}
