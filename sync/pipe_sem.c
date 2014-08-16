#define _GNU_SOURCE

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "pipe_sem.h"


// Initializes a semaphore and sets its initial value.
void pipe_sem_init(pipe_sem_t** sem, int value)
{
    if (value < 0)
    {
        perror("Semaphore value must be greater than or equal to 0\n");
        free(*sem);
        *sem = NULL;
        return;
    }
    
    // Initialize the pipe.
    if (pipe(**sem) != 0)
    {
        perror("Failed to initialize semaphore\n");
        free(*sem);
        *sem = NULL;
        return;
    }

    // Set the value.
    int i;
    for (i = 0; i < value; i++)
    {
        pipe_sem_signal(**sem);
    }
}

// Releases the semaphore resources.
void pipe_sem_dispose(pipe_sem_t sem)
{
    close(sem[0]);
    close(sem[1]);
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
