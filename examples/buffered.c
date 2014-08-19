#include <stdio.h>
#include "../chan/chan.h"

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
