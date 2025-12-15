#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg){
  pthread_t tid = pthread_self();
  printf("%lu\n", (unsigned long)tid);
  return NULL;
}


int main(){
  while(1){
    pthread_t tid;
    int err;
    err = pthread_create(&tid, NULL, mythread, NULL);

    if (err){
      printf("main: pthread_create() failded: %s\n", strerror(err));
      return -1;
    }
  

    //if (pthread_join(tid, NULL) != 0){
    //  printf("ошибка звершения потока\n");
    //}
  }

  return 0;
}
