#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void *mythread(void *arg){
  printf("------new thread\n");
  char* result = (char*)malloc(13*sizeof(char));
  
  strcpy(result, "hello world\n");
  return (void*)result;
}


int main(){
  pthread_t tid;
  int err;
  char* res;
  printf("-----main thread\n");
  err = pthread_create(&tid, NULL, mythread, NULL);


  if (err){
    printf("main: pthread_create() failded: %s\n", strerror(err));
    return -1;
  }


  if (pthread_join(tid, (void**)&res) != 0){
    printf("ошибка звершения потока\n");
  }

  printf("Result from new thread: %s\n", res);
  free(res);

  return 0;
}
