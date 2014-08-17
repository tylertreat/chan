#include "chan.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

chan_t* chan;

void* sender(void* arg)
{
    chan_send(chan, "hello, how are you?");
    chan_send(chan, "world");

    return NULL;
}

void* receiver(void* arg)
{
    void* str;
    printf("before %p\n", str);
    printf("%d\n", chan_recv(chan, &str));
    printf("after %p\n", str);
    printf("%s\n", str);

    void* str2;
    printf("%d\n", chan_recv(chan, &str2));
    printf("%s\n", str2);

    printf("got %s\n", queue_peek(chan->queue));
    return NULL;
}


int main()
{
    chan = chan_init(2);
    pthread_t th;
    pthread_t th3;
    pthread_create(&th, NULL, sender, "hello");
    pthread_join(th, NULL);
    pthread_create(&th3, NULL, receiver, "");
    pthread_join(th3, NULL);

    chan_dispose(chan);
}
