#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void* thread_block_all(void* arg){
    sigset_t set;
    sigfillset(&set);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    printf("all signals blocked in this thread (tid=%lu)\n", pthread_self());
    while(1){
        pause(); 
    }
    return NULL;
}

void sigint_handler(int signo){
    printf("thread catch SIGINT(Ctrl+C)\n");
}


void sigint_handler2(int signo){
    printf("thread_clone catch SIGINT(Ctrl+C)\n");
}
void* thread_sigint(void* arg){
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGINT);

    pthread_sigmask(SIG_SETMASK, &set, NULL); 

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    printf("thread wait SIGINT(Ctrl+C) (tid=%lu)\n", pthread_self());
    while(1){
        pause();
    }
    return NULL;
}

void* thread_sigquit(void* arg){
    sigset_t set;
    int sig;

    sigemptyset(&set);
    sigaddset(&set, SIGQUIT);
    pthread_sigmask(SIG_BLOCK, &set, NULL); 

    printf("thread wait SIGQUIT with sigwait() (Ctrl+\\) (tid=%lu)\n", pthread_self());
    while(1){
        if (sigwait(&set, &sig) == 0){
            printf("thread catch SIGQUIT with sigwait()\n");
        }
    }
    return NULL;
}

void* thread_none(void* arg){
    printf("thread4: no signal handling (tid=%lu)\n", pthread_self());
    while(1){
        pause();
    }
    return NULL;
}

void* thread_sigquit_clone(void* arg){
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGINT);

    pthread_sigmask(SIG_SETMASK, &set, NULL); 

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler2;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    printf("thread_clone wait SIGINT(Ctrl+C) (tid=%lu)\n", pthread_self());
    while(1){
        pause();
    }
    return NULL;
}

int main(){
    pthread_t t1, t2, t3, t4, t5;
    int err;
    sigset_t set;

    sigfillset(&set);
    //pthread_sigmask(SIG_BLOCK, &set, NULL);

    printf("main: pid=%d\n", getpid());

    err = pthread_create(&t1, NULL, thread_block_all, NULL);
    if (err){
        printf("pthread_create t1 failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_create(&t2, NULL, thread_sigint, NULL);
    if (err){
        printf("pthread_create t2 failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_create(&t3, NULL, thread_sigquit, NULL);
    if (err){
        printf("pthread_create t3 failed: %s\n", strerror(err));
        return -1;
    }

    err = pthread_create(&t4, NULL, thread_none, NULL);
    if (err){
        printf("pthread_create t4 failed: %s\n", strerror(err));
        return -1;
    }


    err = pthread_create(&t5, NULL, thread_sigquit_clone, NULL);
    if (err){
        printf("pthread_create t5 failed: %s\n", strerror(err));
        return -1;
    }
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);
    pthread_join(t5, NULL);

    return 0;
}

