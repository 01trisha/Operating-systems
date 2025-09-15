#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int global_var = 0;

void *mythread(void *arg) {
  pthread_t self = pthread_self();
  pthread_t arg_tid = *(pthread_t *)arg;

  int local_var = 0;               // локальная переменная
  static int local_static_var = 0; // локальная статическая
  const int local_const_var = 0;   // локальная константная

  printf("\n--- Поток ---\n");
  printf("PID=%d, PPID=%d, TID=%d\n", getpid(), getppid(), gettid());
  printf("pthread_self=%lu, arg_tid=%lu, equal? %s\n", (unsigned long)self,
         (unsigned long)arg_tid, pthread_equal(self, arg_tid) ? "YES" : "NO");

  printf("Адреса переменных:\n");
  printf("  local_var       = %p\n", (void *)&local_var);
  printf("  local_static_var= %p\n", (void *)&local_static_var);
  printf("  local_const_var = %p\n", (void *)&local_const_var);
  printf("  global_var      = %p\n", (void *)&global_var);

  // робуем изменить
  local_var += 1;
  global_var += 1;

  printf("Значения:\n");
  printf("  local_var=%d (видно только в этом потоке)\n", local_var);
  printf("  global_var=%d (общая переменная)\n", global_var);

  return NULL;
}

int main() {
  pthread_t tids[5];
  int err;

  printf("main [%d %d %d]: старт программы\n", getpid(), getppid(), gettid());

  for (int i = 0; i < 5; i++) {
    err = pthread_create(&tids[i], NULL, mythread, &tids[i]);
    if (err) {
      printf("pthread_create() failed: %s\n", strerror(err));
      return -1;
    }
  }

  for (int i = 0; i < 5; i++) {
    pthread_join(tids[i], NULL);
  }

  printf("\nmain: финальное значение global_var=%d\n", global_var);
  return 0;
}
