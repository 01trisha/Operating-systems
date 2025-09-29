#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

struct person{
    int age;
    char* name;
};

void *mythread(void* arg){
    struct person *p = (struct person *)arg;
    printf("name: %s, age: %d;", p->name, p->age);
    free(p->name);
    return NULL;
}

int main(){
    pthread_t tid;
    pthread_attr_t attr;
    int err;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    struct person *tom = (struct person*)malloc(sizeof(struct person));
    if (tom == NULL){
        printf("malloc failed\n");
        return -1;
    }
    
    tom->age=10;
    tom->name=strdup("Tom");

    err = pthread_create(&tid, &attr, mythread, tom);
    if (err){
        printf("main: pthread_create failed: %s\n", strerror(err));
        free(tom);
        return -1;
    }

    sleep(1);

    pthread_attr_destroy(&attr);

    return 0;
}

