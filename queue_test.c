#include "queue.h"
#include "sync/reentrant_lock.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>


void* producer(void* queue)
{
    int i;
    for (i = 0; i < 100; i++)
    {
        queue_add(queue, (void*) (intptr_t) i + 1);
    }
    printf("done producing\n");
    return NULL;
}

void* consumer(void* queue)
{
    int recv;
    printf("start consuming\n");
    while ((recv = (int) queue_remove(queue)))
    {
        printf("%d: %d\n", (int) pthread_self(), recv);
    }
    printf("done consuming\n");
    return NULL;
}

int main()
{
    queue_t* queue = queue_init(10);
    int count = 5;
    pthread_t th[count];

    int i;
    for (i = 0; i < count; i++)
    {
        pthread_create(&th[i], NULL, i == 0 ? producer : consumer, &queue);
        if (i == 0)
        {
            //sleep(2);
        }
    }

    for (i = 0; i < count; i++)
    {
        pthread_join(th[i], NULL);
    }

    printf("%d\n", queue->size);
    queue_dispose(queue);
}
