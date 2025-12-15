// test.c
#include "uthread.h"
#include <stdio.h>
#include <stdlib.h>

void *thread_func(void *arg) {
    int id = *(int*)arg;
    for (int i = 0; i < 5; i++) {
        printf("поток %d итерация %d\n", id, i);
        uthread_yield();
    }
    return NULL;
}

int main() {
    uthread_t th1, th2, th3;
    int arg1 = 1, arg2 = 2, arg3 = 3;
    if (uthread_create(&th1, thread_func, &arg1) != 0) {
        fprintf(stderr, "failed to create thread 1\n");
        exit(1);
    }
    if (uthread_create(&th2, thread_func, &arg2) != 0) {
        fprintf(stderr, "failed to create thread 2\n");
        exit(1);
    }
    if (uthread_create(&th3, thread_func, &arg3) != 0) {
        fprintf(stderr, "failed to create thread 3\n");
        exit(1);
    }
    void *ret;
    uthread_join(th1, &ret);
    uthread_join(th2, &ret);
    uthread_join(th3, &ret);
    return 0;
}
