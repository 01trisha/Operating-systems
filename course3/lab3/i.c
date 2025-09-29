#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

struct person {
    int age;
    char* name;
};

void *print_thread(void *arg) {
    struct person *p = (struct person*)arg;

    sleep(5);

    printf("Detached: name=%s, age=%d\n", p->name, p->age);
    return NULL;
}

void *overwrite_thread(void *arg) {
    struct person *p = (struct person*)arg;

    p->age = 999;
    p->name = "HACKED";

    return NULL;
}

int main() {
    pthread_t tid1, tid2;
    pthread_attr_t attr;
    int err;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    struct person tom;
    tom.age = 10;
    tom.name = "Tom";

    err = pthread_create(&tid1, &attr, print_thread, &tom);
    if (err) {
        fprintf(stderr, "pthread_create print_thread: %s\n", strerror(err));
        return 1;
    }
    //char buffer[1024];
    //memset(buffer, 'X', sizeof(buffer));
    //pthread_exit(NULL);
    usleep(100000); 


    err = pthread_create(&tid2, NULL, overwrite_thread, &tom);
    if (err) {
        fprintf(stderr, "pthread_create overwrite_thread: %s\n", strerror(err));
        return 1;
    }

    pthread_join(tid2, NULL);

    sleep(7);

    pthread_attr_destroy(&attr);
    return 0;
}

