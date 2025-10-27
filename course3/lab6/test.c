#include <stdio.h>
#include <stdlib.h>
#include "mythread.h"
#include <unistd.h>
#include <string.h>

void *example_thread(void* arg){
    int id = (int)(long)arg;

    printf("thread %d in started: pid = %d\n", id, getpid());
    sleep(1);
    printf("thread %d is finished\n", id);

    return NULL;
}

void *return_value_thread(void* arg) {
    int id = (int)(long)arg;
    printf("return thread %d working\n", id);
    return (void*)(long)(id * 1000);
}

void *stack_usage_thread(void* arg) {
    int id = (int)(long)arg;
    char buffer[16384];
    for (int i = 0; i < sizeof(buffer); i++) {
        buffer[i] = (char)(i + id);
    }
    printf("stack thread %d used %zu bytes\n", id, sizeof(buffer));
    return (void*)(long)id;
}

int main(){
    mythread_t t1, t2, ret_thread, stack_thread, single_thread;
    int err;
    void *retval;
    
    printf("---correctly create test\n");
    err = mythread_create(&t1, example_thread, (void*)1);
    if (err){
        printf("failed mythread_create: %s\n", strerror(err));
        return -1;
    }
    
    sleep(1);
    err = mythread_create(&t2, example_thread, (void*)2);
    if (err){
        printf("failed mythread_create: %s\n", strerror(err));
        return -1;
    }


    mythread_join(&t1, NULL);
    mythread_join(&t2, NULL);
     

    printf("---invalid create test\n");
    if (mythread_create(NULL, example_thread, (void *)3) != 0){
        printf("Error handled correctly.\n");
    }

    printf("---return value test\n");
    err = mythread_create(&ret_thread, return_value_thread, (void*)3);
    if (!err) {
        mythread_join(&ret_thread, &retval);
        printf("thread returned: %ld\n", (long)retval);
    }
    
    printf("---stack usege test\n");
    err = mythread_create(&stack_thread, stack_usage_thread, (void*)4);
    if (!err) {
        mythread_join(&stack_thread, &retval);
        printf("stack thread returned: %ld\n", (long)retval);
    }

    printf("---double join test\n");
    err = mythread_create(&single_thread, example_thread, (void*)5);
    if (!err) {
        mythread_join(&single_thread, NULL);
        printf("first join completed\n");
        
        if (mythread_join(&single_thread, NULL) != 0) {
            perror("double join correctly failed\n");
        } else {
            printf("double join unexpectedly succeeded\n");
        }
    }

    return 0;
}
