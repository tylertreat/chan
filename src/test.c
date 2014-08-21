
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "countdown_latch.h"

countdown_latch_t* latch;

void* foo()
{
    printf("countdown A\n");
    countdown_latch_count_down(latch);
    return NULL;
}

void* bar()
{
    sleep(3);
    printf("countdown B\n");
    countdown_latch_count_down(latch);
    return NULL;
}

int main() 
{
    latch = countdown_latch_init(2);

    pthread_t th;
    pthread_create(&th, NULL, foo, NULL);

    pthread_t th2;
    pthread_create(&th2, NULL, bar, NULL);


    countdown_latch_await(latch);

    printf("done\n");
}
