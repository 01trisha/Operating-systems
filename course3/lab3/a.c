#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

struct person{
    int age;
    char* name;
};

void *mythread(void* arg){
    struct person *p = (struct person *)arg;
    printf("name: %s, age: %d;", p->name, p->age);
    return NULL;
}

int main(){
    struct person tom = {10, "Tom"};
    pthread_t tid;
    int err;

    err = pthread_create(&tid, NULL, mythread, &tom);
    if (err){
        printf("main: pthread_create failed: %s\n", strerror(err));
        return -1;
    }

    pthread_join(tid, NULL);


}

