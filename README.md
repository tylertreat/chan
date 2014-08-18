chan
====

Pure C implementation of Go channels.

## Unbuffered

Unbuffered channels provide both a mechanism for communication as well as synchronization. When data is sent into the channel, the sender blocks until a receiver is ready. Likewise, a receiver will block until a sender is ready.

```c
#include <pthread.h>
#include <stdio.h>
#include "chan.h"

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
    printf("%s\n", msg);

    // Clean up channel.
    chan_dispose(chan);
}
```

With an unbuffered channel, the sender and receiver are synchronized, so the above program will print `ping`.

## Buffered

Buffered channels accept a limited number of values without a corresponding receiver for those values. Sending data will not block unless the channel is full. Receiving data will block only if the channel is empty.

```c
#include <stdio.h>
#include "chan.h"

int main()
{
    // Initialize buffered channel with a capacity of 2.
    chan_t* chan = chan_init(2);

    // Send up to 2 values without receiver.
    chan_send(chan, "buffered");
    chan_send(chan, "channel");

    // Later receive the values.
    void* msg;
    chan_recv(chan, &msg);
    printf("%s\n", msg);

    chan_recv(chan, &msg);
    printf("%s\n", msg);

    // Clean up channel.
    chan_dispose(chan);
}
```

The above program will print `buffered` and then `channel`. The sends do not block because the channel has a capacity of 2. Sending more after that would block until values were received.
