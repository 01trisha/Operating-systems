#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

void cleanup(void *arg) {
    char *str = (char *)arg;
    printf("\n[cleanup] очистка памяти\n");
    free(str);
}

void* mythread(void* arg){
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    char *msg = malloc(32);
    if (!msg) {
        perror("malloc");
        pthread_exit(NULL);
    }

    snprintf(msg, 32, "hello world");

    pthread_cleanup_push(cleanup, msg);

    while(1){
        printf("%s\n", msg);
        usleep(200000);
        
        pthread_testcancel();
    }
    
    pthread_cleanup_pop(1);
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
    printf("завершено корректно\n");
    return 0;
}
