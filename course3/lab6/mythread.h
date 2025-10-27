#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <sys/types.h>

struct thread_arg;

typedef struct mythread{
    pid_t thread_id;
    void* stack;
    void* thread_arg;
}mythread_t;

int mythread_create(mythread_t *thread, void *(*start_routine)(void *), void *arg);
int mythread_join(mythread_t *thread, void **return_value);

#endif 
