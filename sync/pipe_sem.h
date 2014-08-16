#ifndef pipe_sem_h
#define pipe_sem_h

// Defines a semaphore where the value specifies the number of concurrent
// accesses allowed.
typedef struct
{
    int fd[2];
    int value;
} pipe_sem_t;

// Initializes a semaphore and sets its initial value. Pointer will be NULL if
// initialization fails.
void pipe_sem_init(pipe_sem_t** sem, int value);

// Releases the semaphore resources.
void pipe_sem_dispose(pipe_sem_t* sem);

// Performs a wait operation on the semaphore.
void pipe_sem_wait(pipe_sem_t* sem);

// Performs a signal operation on the semaphore.
void pipe_sem_signal(pipe_sem_t* sem);

#endif
