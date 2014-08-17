#include "chan.h"
#include "sync/reentrant_lock.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>


void* producer(void* chan)
{
    int i;
    for (i = 0; i < 100; i++)
    {
        chan_send(chan, (void*) (intptr_t) i + 1);
    }
    printf("done producing\n");
    return NULL;
}

void* consumer(void* chan)
{
    int recv;
    printf("start consuming\n");
    while ((recv = (int) chan_recv(chan)))
    {
        printf("%d: %d\n", (int) pthread_self(), recv);
    }
    printf("done consuming\n");
    return NULL;
}

int main()
{
    chan_t* chan = chan_init(10);
    int count = 5;
    pthread_t th[count];

    int i;
    for (i = 0; i < count; i++)
    {
        pthread_create(&th[i], NULL, i == 0 ? producer : consumer, &chan);
        if (i == 0)
        {
            //sleep(2);
        }
    }

    for (i = 0; i < count; i++)
    {
        pthread_join(th[i], NULL);
    }

    printf("%d\n", chan_size(chan));
    chan_dispose(chan);
}
