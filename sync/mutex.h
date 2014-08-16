#ifndef mutex_h
#define mutex_h

#include "pipe_sem.h"

// Defines a mutex whose lock can be acquired once.
typedef struct
{
    pipe_sem_t* sem;
} mutex_t;

// Allocates and returns a new mutex. Returns NULL if initialization failed.
mutex_t* mutex_init();

// Releases the mutex resources.
void mutex_dispose(mutex_t* mu);

// Acquires a lock on the mutex.
void mutex_lock(mutex_t* mu);

// Releases the lock on the mutex.
void mutex_unlock(mutex_t* mu);

#endif
