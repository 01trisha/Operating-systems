#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg){
  pthread_t tid;
  printf("%lu\n", (unsigned long)tid);
  return NULL;
}


int main(){

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  while(1){
    pthread_t tid;
    int err;
    err = pthread_create(&tid, &attr, mythread, NULL);

    if (err){
      printf("main: pthread_create() failded: %s\n", strerror(err));
      return -1;
    }

    pthread_attr_destroy(&attr);
  }

  return 0;
}
