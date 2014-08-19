#define _GNU_SOURCE

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "reentrant_lock.h"


// Allocates and returns a new reentrant lock. Returns NULL if initialization
// failed.
reentrant_lock_t* reentrant_lock_init()
{
    mutex_t* mu = mutex_init();
    if (!mu)
    {
        return NULL;
    }

    mutex_t* owner_mu = mutex_init();
    if (!owner_mu)
    {
        return NULL;
    }

    reentrant_lock_t* lock = (reentrant_lock_t*) malloc(sizeof(reentrant_lock_t));
    if (!lock)
    {
        perror("Failed to allocate lock");
        return NULL;
    }

    lock->mu = mu;
    lock->owner = NULL;
    lock->owner_mu = owner_mu;
    lock->count = 0;
    return lock;
}

// Releases the lock resources.
void reentrant_lock_dispose(reentrant_lock_t* lock)
{
    mutex_dispose(lock->owner_mu);
    mutex_dispose(lock->mu);
    free(lock);
}

// Acquires the lock. This will block if the calling thread does not own the
// lock. Once the lock has been acquired, its counter will be incremented.
void reentrant_lock(reentrant_lock_t* lock)
{
    pthread_t tid = pthread_self();
    mutex_lock(lock->owner_mu);
    if (pthread_equal(lock->owner, tid))
    {
        // Reentrant acquire.
        lock->count++;
        mutex_unlock(lock->owner_mu);
        return;
    }

    mutex_unlock(lock->owner_mu);
    mutex_lock(lock->mu);
    mutex_lock(lock->owner_mu);
    lock->owner = tid;
    lock->count++;
    mutex_unlock(lock->owner_mu);
}

// Decrements the lock's counter. If the counter reaches zero, the lock is
// released.
void reentrant_unlock(reentrant_lock_t* lock)
{
    mutex_lock(lock->owner_mu);
    lock->count--;
    if (lock->count == 0)
    {
        mutex_unlock(lock->mu);
        lock->owner = NULL;
    }
    mutex_unlock(lock->owner_mu);
}
