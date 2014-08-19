#define _GNU_SOURCE

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "pipe_sem.h"


// Initializes a semaphore and sets its initial value. Initial value may not be
// less than 0. Sets errno and returns NULL if initialization failed.
pipe_sem_t* pipe_sem_init(int value)
{
    if (value < 0)
    {
        errno = EINVAL;
        return NULL;
    }

    // Allocate semaphore.
    pipe_sem_t* sem = (pipe_sem_t*) malloc(sizeof(pipe_sem_t));
    if (!sem)
    {
        errno = ENOMEM;
        return NULL;
    }
    
    // Initialize the pipe.
    if (pipe(*sem) != 0)
    {
        free(sem);
        return NULL;
    }

    // Set the value.
    int i;
    for (i = 0; i < value; i++)
    {
        pipe_sem_signal(*sem);
    }

    return sem;
}

// Releases the semaphore resources.
void pipe_sem_dispose(pipe_sem_t* sem)
{
    close(*sem[0]);
    close(*sem[1]);
    free(sem);
}

// Performs a wait operation on the semaphore.
void pipe_sem_wait(pipe_sem_t sem)
{
    // Block the thread by reading from pipe.
    char buf[1];
    read(sem[0], buf, 1);
}

// Performs a signal operation on the semaphore.
void pipe_sem_signal(pipe_sem_t sem)
{
    write(sem[1], "1", 1);
}
