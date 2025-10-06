#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void* mythread(void* arg){
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    while(1){
        printf("hello from mythread!\n");
        usleep(200000);
    }
    
    return NULL;
}

int main(){
    pthread_t tid;
    int err;

    err = pthread_create(&tid, NULL, mythread, NULL);
    if (err){
        printf("main: pthread_create failed: %s\n", strerror(err));
        return -1;
    }

    sleep(2);

    pthread_cancel(tid);
    
    pthread_join(tid, NULL);
    return 0;
}
