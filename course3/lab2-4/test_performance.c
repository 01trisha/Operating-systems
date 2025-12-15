#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define COUNT_TO 2000000

int counter_no_sync = 0;

void *thread_no_sync(void *arg) {
    for (int i = 0; i < COUNT_TO / 2; i++) {
        counter_no_sync++;
    }
    return NULL;
}

int counter_mutex = 0;
pthread_mutex_t mutex;

void *thread_mutex(void *arg) {
    for (int i = 0; i < COUNT_TO / 2; i++) {
        pthread_mutex_lock(&mutex);
        counter_mutex++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;

    //printf("ьез синхронизации: ");
    //counter_no_sync = 0;
    //pthread_create(&t1, NULL, thread_no_sync, NULL);
    //pthread_create(&t2, NULL, thread_no_sync, NULL);
    //pthread_join(t1, NULL);
    //pthread_join(t2, NULL);
    //printf("%d\n", counter_no_sync);

    //printf("c мутексом: ");
    pthread_mutex_init(&mutex, NULL);
    counter_mutex = 0;
    pthread_create(&t1, NULL, thread_mutex, NULL);
    pthread_create(&t2, NULL, thread_mutex, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("pthread_mutex %d\n", counter_mutex);

    pthread_mutex_destroy(&mutex);

    return 0;
}
