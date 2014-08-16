#ifndef pipe_sem_h
#define pipe_sem_h

// Defines a semaphore using a pipe. The number of bytes in the pipe determines
// the semaphore value.
typedef int pipe_sem_t[2];

// Initializes a semaphore and sets its initial value. Pointer will be NULL if
// initialization fails.
void pipe_sem_init(pipe_sem_t** sem, int value);

// Releases the semaphore resources.
void pipe_sem_dispose(pipe_sem_t sem);

// Performs a wait operation on the semaphore.
void pipe_sem_wait(pipe_sem_t sem);

// Performs a signal operation on the semaphore.
void pipe_sem_signal(pipe_sem_t sem);

#endif
