#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include "../src/chan.h"

chan_t* jobs;
chan_t* done;

void* worker()
{
    // Process jobs until channel is closed.
    void* job;
    while (chan_recv(jobs, &job) == 0)
    {
        printf("received job %ld\n", (long) job);
    }

    // Notify that all jobs were received.
    printf("received all jobs\n");
    chan_send(done, "1");
    return NULL;
}

int main()
{
    // Initialize channels.
    jobs = chan_init(5);
    done = chan_init(0);

    pthread_t th;
    pthread_create(&th, NULL, worker, NULL);

    // Send 3 jobs over the jobs channel then close it.
    int i;
    for (i = 1; i <= 3; i++)
    {
        chan_send(jobs, (void*) (uintptr_t) i);
        printf("sent job %d\n", i);
    }
    chan_close(jobs);
    printf("sent all jobs\n");

    // Wait for all jobs to be received.
    chan_recv(done, NULL);

    // Clean up channels.
    chan_dispose(jobs);
    chan_dispose(done);
}
