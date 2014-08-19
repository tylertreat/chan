#ifndef reentrant_lock_h
#define reentrant_lock_h

#include <pthread.h>

#include "mutex.h"


// Defines a reentrant lock where the owning thread can reacquire the lock. The
// lock must be released for each acquisition or else it will deadlock.
typedef struct
{
    mutex_t*  mu;
    pthread_t owner;
    mutex_t*  owner_mu;
    int       count;
} reentrant_lock_t;

// Allocates and returns a new reentrant lock. Returns NULL if initialization
// failed.
reentrant_lock_t* reentrant_lock_init();

// Releases the lock resources.
void reentrant_lock_dispose(reentrant_lock_t* lock);

// Acquires the lock. This will block if the calling thread does not own the
// lock. Once the lock has been acquired, its counter will be incremented.
void reentrant_lock(reentrant_lock_t* lock);

// Decrements the lock's counter. If the counter reaches zero, the lock is
// released.
void reentrant_unlock(reentrant_lock_t* lock);

#endif
