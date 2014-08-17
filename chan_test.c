#include "chan.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>


void* foo(void* chan)
{
    printf("sending\n");
    chan_send(chan, pthread_self());
    printf("sent\n");
    return NULL;
}


int main()
{
    chan_t* chan = chan_init(0);
    pthread_t th;
    pthread_t th2;
    pthread_create(&th, NULL, foo, chan);
    pthread_create(&th2, NULL, foo, chan);
    //pthread_join(th, NULL);
    //pthread_join(th2, NULL);

    sleep(2);
    printf("recv\n");
    printf("%d\n", (int) chan_recv(chan));
    sleep(1);
    printf("size %d\n", chan_size(chan));
    printf("%d\n", (int) chan_recv(chan));
    printf("size %d\n", chan_size(chan));
    chan_dispose(chan);
}
