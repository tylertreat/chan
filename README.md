chan
====

Pure C implementation of Go channels.

## Unbuffered Channels

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

## Buffered Channels

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

## Closing Channels

When a channel is closed, no more values can be sent on it. Receiving on a closed channel will return an indication code that the channel has been closed. This can be useful to communicate completion to the channel’s receivers. If the closed channel is buffered, values will be received on it until empty.

```c
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include "chan.h"

chan_t* jobs;
chan_t* done;

void* worker()
{
    // Process jobs until channel is closed.
    void* job;
    while (chan_recv(jobs, &job) == 0)
    {
        printf("received job %d\n", (int) job);
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
```

This program will print:

```
sent job 1
received job 1
sent job 2
received job 2
sent job 3
received job 3
sent all jobs
received all jobs
```

## Select Statements

Select statements choose which of a set of possible send or receive operations will proceed. They also provide a way to perform non-blocking sends and receives. Selects are particularly useful for multiplexing communication over several channels.

```c
#include <stdio.h>
#include "chan.h"

chan_t* messages;
chan_t* signals;

int main()
{
    // Initialize channels.
    messages = chan_init(0);
    signals = chan_init(0);
    void *msg;

    // This is a non-blocking receive. If a value is available on messages,
    // select will take the messages (0) case with that value. If not, it will
    // immediately take the default case.
    switch(chan_select(&messages, 1, &msg, NULL, 0, NULL))
    {
        case 0:
            printf("received message %s\n", msg);
            break;
        default:
            printf("no message received\n");
    }

    // A non-blocking send works similarly.
    msg = "foo";
    switch(chan_select(NULL, 0, NULL, &messages, 1, &msg))
    {
        case 0:
            printf("sent message %s\n", msg);
            break;
        default:
            printf("no message sent\n");
    }

    // We can use multiple cases above the default clause to implement a
    // multi-way non-blocking select. Here we attempt non-blocking receives on
    // both messages and signals.
    chan_t* chans[2] = {messages, signals};
    switch(chan_select(chans, 2, &msg, NULL, 0, NULL))
    {
        case 0:
            printf("received message %s\n", msg);
            break;
        case 1:
            printf("received signal %s\n", msg);
            break;
        default:
            printf("no activity\n");
    }

    // Clean up channels.
    chan_dispose(messages);
    chan_dispose(signals);
}
```

This program will print:

```
no message received
no message sent
no activity
```
