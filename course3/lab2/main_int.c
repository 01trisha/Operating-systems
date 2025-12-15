#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg){
  printf("hello from new thread\n");
  int* result = malloc(sizeof(int));
  
  *result = 42;
  return (void*)result;
}


int main(){
  pthread_t tid;
  int err;
  int* res;
  printf("hello from main thread\n");
  err = pthread_create(&tid, NULL, mythread, NULL);


  if (err){
    printf("main: pthread_create() failded: %s\n", strerror(err));
    return -1;
  }


  if (pthread_join(tid, (void**)&res) != 0){
    printf("ошибка звершения потока\n");
  }

  printf("Result from new thread: %d\n", *res);

  return 0;
}
