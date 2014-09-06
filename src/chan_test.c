#undef __STRICT_ANSI__

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "chan.h"

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(x)
#endif

int passed = 0;

void assert_true(int expression, chan_t* chan, char* msg)
{
    if (!expression)
    {
        chan_dispose(chan);
        fprintf(stderr, "Assertion failed: %s\n", msg);
        exit(1);
    }
}

void pass()
{
    printf(".");
    passed++;
}

void test_chan_init_buffered()
{
    size_t size = 5;
    chan_t* chan = chan_init(size);

    assert_true(chan->queue != NULL, chan, "Queue is NULL");
    assert_true((size_t) chan->queue->capacity == size, chan, "Size is not 5");
    assert_true(!chan->closed, chan, "Chan is closed");

    chan_dispose(chan);
    pass();
}

void test_chan_init_unbuffered()
{
    chan_t* chan = chan_init(0);

    assert_true(chan->queue == NULL, chan, "Queue is not NULL");
    assert_true(!chan->closed, chan, "Chan is closed");

    chan_dispose(chan);
    pass();
}

void test_chan_init()
{
    test_chan_init_buffered();
    test_chan_init_unbuffered();
}

void test_chan_close()
{
    chan_t* chan = chan_init(0);

    assert_true(!chan->closed, chan, "Chan is closed");
    assert_true(!chan_is_closed(chan), chan, "Chan is closed");
    assert_true(chan_close(chan) == 0, chan, "Close failed");
    assert_true(chan->closed, chan, "Chan is not closed");
    assert_true(chan_close(chan) == -1, chan, "Close succeeded");
    assert_true(chan_is_closed(chan), chan, "Chan is not closed");
    
    chan_dispose(chan);
    pass();
}

void test_chan_send_buffered()
{
    chan_t* chan = chan_init(1);
    void* msg = "foo";

    assert_true(chan_size(chan) == 0, chan, "Queue is not empty");
    assert_true(chan_send(chan, msg) == 0, chan, "Send failed");
    assert_true(chan_size(chan) == 1, chan, "Queue is empty");
    
    chan_dispose(chan);
    pass();
}

void* receiver(void* chan)
{
    void* msg;
    chan_recv(chan, &msg);
    return NULL;
}

void test_chan_send_unbuffered()
{
    chan_t* chan = chan_init(0);
    void* msg = "foo";
    pthread_t th;
    pthread_create(&th, NULL, receiver, chan);

    assert_true(chan_size(chan) == 0, chan, "Chan size is not 0");
    assert_true(!chan->pipe->sender, chan, "Chan has sender");
    assert_true(chan_send(chan, msg) == 0, chan, "Send failed");
    assert_true(!chan->pipe->sender, chan, "Chan has sender");
    assert_true(chan_size(chan) == 0, chan, "Chan size is not 0");

    chan_dispose(chan);
    pass();
}

void test_chan_send()
{
    test_chan_send_buffered();
    test_chan_send_unbuffered();
}

void test_chan_recv_buffered()
{
    chan_t* chan = chan_init(1);
    void* msg = "foo";

    assert_true(chan_size(chan) == 0, chan, "Queue is not empty");
    chan_send(chan, msg);
    assert_true(chan_size(chan) == 1, chan, "Queue is empty");
    void* received;
    assert_true(chan_recv(chan, &received) == 0, chan, "Recv failed");
    assert_true(msg == received, chan, "Messages are not equal");
    assert_true(chan_size(chan) == 0, chan, "Queue is not empty");
    
    chan_dispose(chan);
    pass();
}

void* sender(void* chan)
{
    chan_send(chan, "foo");
    return NULL;
}

void test_chan_recv_unbuffered()
{
    chan_t* chan = chan_init(0);
    pthread_t th;
    pthread_create(&th, NULL, sender, chan);

    assert_true(chan_size(chan) == 0, chan, "Chan size is not 0");
    assert_true(!chan->pipe->reader, chan, "Chan has reader");
    void *msg;
    assert_true(chan_recv(chan, &msg) == 0, chan, "Recv failed");
    assert_true(strcmp(msg, "foo") == 0, chan, "Messages are not equal");
    assert_true(!chan->pipe->reader, chan, "Chan has reader");
    assert_true(chan_size(chan) == 0, chan, "Chan size is not 0");

    chan_dispose(chan);
    pass();
}

void test_chan_recv()
{
    test_chan_recv_buffered();
    test_chan_recv_unbuffered();
}

void test_chan_select_recv()
{
    chan_t* chan1 = chan_init(0);
    chan_t* chan2 = chan_init(1);
    chan_t* chans[2] = {chan1, chan2};
    void* recv;
    void* msg = "foo";

    chan_send(chan2, msg);

    switch(chan_select(chans, 2, &recv, NULL, 0, NULL))
    {
        case 0:
            chan_dispose(chan1);
            chan_dispose(chan2);
            fprintf(stderr, "Received on wrong channel\n");
            exit(1);
        case 1:
            if (strcmp(recv, msg) != 0)
            {
                chan_dispose(chan1);
                chan_dispose(chan2);
                fprintf(stderr, "Messages are not equal\n");
                exit(1);
            }
            recv = NULL;
            break;
        default:
            chan_dispose(chan1);
            chan_dispose(chan2);
            fprintf(stderr, "Received on no channels\n");
            exit(1);
    }

    pthread_t th;
    pthread_create(&th, NULL, sender, chan1);
    sleep(1);

    switch(chan_select(chans, 2, &recv, NULL, 0, NULL))
    {
        case 0:
            if (strcmp(recv, "foo") != 0)
            {
                chan_dispose(chan1);
                chan_dispose(chan2);
                fprintf(stderr, "Messages are not equal\n");
                exit(1);
            }
            recv = NULL;
            break;
        case 1:
            chan_dispose(chan1);
            chan_dispose(chan2);
            fprintf(stderr, "Received on wrong channel\n");
            exit(1);
        default:
            chan_dispose(chan1);
            chan_dispose(chan2);
            fprintf(stderr, "Received on no channels\n");
            exit(1);
    }

    switch(chan_select(&chan2, 1, &msg, NULL, 0, NULL))
    {
        case 0:
            chan_dispose(chan1);
            chan_dispose(chan2);
            fprintf(stderr, "Received on channel\n");
            exit(1);
        default:
            break;
    }

    chan_dispose(chan1);
    chan_dispose(chan2);
    pass();
}

void test_chan_select_send()
{
    chan_t* chan1 = chan_init(0);
    chan_t* chan2 = chan_init(1);
    chan_t* chans[2] = {chan1, chan2};
    void* msg[] = {"foo", "bar"};

    switch(chan_select(NULL, 0, NULL, chans, 2, msg))
    {
        case 0:
            chan_dispose(chan1);
            chan_dispose(chan2);
            fprintf(stderr, "Sent on wrong channel");
            exit(1);
        case 1:
            break;
        default:
            chan_dispose(chan1);
            chan_dispose(chan2);
            fprintf(stderr, "Sent on no channels");
            exit(1);
    }

    void* recv;
    chan_recv(chan2, &recv);
    if (strcmp(recv, "bar") != 0)
    {
        chan_dispose(chan1);
        chan_dispose(chan2);
        fprintf(stderr, "Messages are not equal");
        exit(1);
    }

    chan_send(chan2, "foo");

    pthread_t th;
    pthread_create(&th, NULL, receiver, chan1);
    sleep(1);

    switch(chan_select(NULL, 0, NULL, chans, 2, msg))
    {
        case 0:
            break;
        case 1:
            chan_dispose(chan1);
            chan_dispose(chan2);
            fprintf(stderr, "Sent on wrong channel");
            exit(1);
        default:
            chan_dispose(chan1);
            chan_dispose(chan2);
            fprintf(stderr, "Sent on no channels");
            exit(1);
    }

    switch(chan_select(NULL, 0, NULL, &chan1, 1, msg[0]))
    {
        case 0:
            chan_dispose(chan1);
            chan_dispose(chan2);
            fprintf(stderr, "Sent on channel");
            exit(1);
        default:
            break;
    }

    chan_dispose(chan1);
    chan_dispose(chan2);
    pass();
}

void test_chan_select()
{
    test_chan_select_recv();
    test_chan_select_send();
}

void test_chan_int()
{
    chan_t* chan = chan_init(1);
    int s = 12345, r = 0;
    chan_send_int(chan, s);
    chan_recv_int(chan, &r);
    assert_true(s == r, chan, "Wrong value of int(12345)");

    chan_dispose(chan);
    pass();
}

void test_chan_double()
{
    chan_t* chan = chan_init(1);
    double s = 123.45, r = 0;
    chan_send_double(chan, s);
    chan_recv_double(chan, &r);
    assert_true(s == r, chan, "Wrong value of double(123.45)");

    chan_dispose(chan);
    pass();
}

void test_chan_buf()
{
    chan_t* chan = chan_init(1);
    char s[256], r[256];
    strcpy(s, "hello world");
    chan_send_buf(chan, s, sizeof(s));
    strcpy(s, "Hello World");
    chan_recv_buf(chan, &r, sizeof(s));
    assert_true(memcmp(s, r, sizeof(s)), chan, "Wrong value of buf");

    chan_dispose(chan);
    pass();
}

int main()
{
    test_chan_init();
    test_chan_close();
    test_chan_send();
    test_chan_recv();
    test_chan_select();
    test_chan_int();
    test_chan_double();
    test_chan_buf();
    printf("\n%d passed\n", passed);
    return 0;
}
