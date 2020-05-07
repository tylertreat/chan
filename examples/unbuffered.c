#include <pthread.h>
#include <stdio.h>
#include "../src/chan.h"

chan_t* chan;

void* ping()
{
    // Send blocks until receiver is ready.
    chan_send(chan, "ping");
    return NULL;
}

int main()
{
    // Initialize unbuffered channel.
    chan = chan_init(0);

    pthread_t th;
    pthread_create(&th, NULL, ping, NULL);

    // Receive blocks until sender is ready.
    void* msg;
    chan_recv(chan, &msg);
    printf("%s\n", (char *)msg);

    // Clean up channel.
    chan_dispose(chan);
}
