#define _GNU_SOURCE
#include "mythread.h"
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#define STACK_SIZE 64*1024

struct thread_arg {
    void *(*func)(void *);
    void *arg;
    void *return_value;
};

static int thread_wrapper(void *varg) {
    struct thread_arg *t = (struct thread_arg *)varg;
    t->return_value = t->func(t->arg);
    return 0;
}

int mythread_create(mythread_t *thread,void *(*start_routine)(void *), void *arg){
    if (!thread || !start_routine){
        fprintf(stderr, "mythread_create: invalid arguments\n");
        return EINVAL;
    }

    thread->stack = malloc(STACK_SIZE);
    if(!thread->stack){
        perror("malloc");
        return ENOMEM;
    }


    struct thread_arg *targ = mmap(NULL, sizeof(*targ), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!targ) {
        free(thread->stack);
        perror("mmap");
        return ENOMEM;
    }

    targ->func = start_routine;
    targ->arg = arg;
    targ->return_value = NULL;

    void* stack_top = (char*)thread->stack + STACK_SIZE;

    int flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD;
    pid_t pid = clone(thread_wrapper, stack_top, flags, targ);

    if(pid == -1){
        perror("clone");
        free(thread->stack);
        munmap(targ, sizeof(*targ));
        return errno;
    }

    thread->thread_id = pid;
    thread->thread_arg = targ;
    return 0;
}

int mythread_join(mythread_t *thread, void **return_value){
    if (!thread || thread->thread_id == 0) {
        return EINVAL;
    }

    int status;
    pid_t w = waitpid(thread->thread_id, &status, 0);

    if (w == -1){
        if (errno == ECHILD) {
            return ESRCH;
        }
        perror("waitpid");
        return errno;
    }


    if (return_value && thread->thread_arg){
        struct thread_arg *targ = (struct thread_arg *)thread->thread_arg;
        *return_value = targ->return_value;
    }

    if (thread->thread_arg) {
        munmap(thread->thread_arg, sizeof(struct thread_arg));
    }
    free(thread->stack);
     
    return 0;
}
    
//int mythread_cancel(){
//   result = kill(tid, SIGKILL)
//
//   if res = -1 
//          то 
//              или уже завершился и отправляем THREAD_ALREADY_FINISHED
//          иначе
//              какая то системная ошибка -> отправляем ее
//
//     return 0;
//} 
