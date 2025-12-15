// uthread.h
#ifndef UTHREAD_H
#define UTHREAD_H
#include <ucontext.h>
#define PAGE 4096
#define STACK_SIZE (PAGE * 16)
#define MAX_THREADS_COUNT 64

typedef enum {
  UTHREAD_READY,
  UTHREAD_RUNNING,
  UTHREAD_FINISHED
} uthread_state_t;

typedef struct uthread {
  int uthread_id;
  void *(*start_routine)(void *);
  void *arg;
  void *retval;
  ucontext_t ucontext;
  void *stack;
  uthread_state_t state;
  struct uthread *next;
} uthread_struct_t;

typedef uthread_struct_t *uthread_t;

int uthread_create(uthread_t *thread, void *(*start_routine)(void *),
                   void *arg);
int uthread_join(uthread_t thread, void **retval);

void uthread_yield(void);
void uthread_exit(void *retval);
#endif
