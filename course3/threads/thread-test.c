#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#define N 5

int global_var = 0;

void *mythread(void *arg) {
	printf("thread-----------------------------------\n\n");
	pthread_t self = pthread_self();
	pthread_t arg_tid = *(pthread_t *)arg;
	printf(" pid: %d\n ppid: %d\n tid: %d\n self: %lu\n 1st arg pt_create: %lu\n equal self & tid? %s\n equal self & 1st arg? %s\n", getpid(), getppid(), gettid(), (unsigned long) self, (unsigned long)arg_tid, pthread_equal(self, gettid()) ? "yes" : "no", pthread_equal(self, arg_tid) ? "yes" : "no");

	int local_var = 0;
	static int local_static_var = 0;
	const int local_const_var = 0;


	printf("\nадреса переменных\n");
	printf("local var: %p\n", (void *)&local_var);
	printf("local static var: %p\n", (void *)&local_static_var);
	printf("local_const_var: %p\n", (void *)&local_const_var);
	printf("global_var: %p\n\n");

	local_var += 1;
	global_var += 1;

	printf("измененные переменные\n");
	printf("local var = %d\n", local_var);
	printf("global_var = %d\n\n", global_var);
	return NULL;
}

int main() {
	pthread_t tid[N];
	int err;

	printf("main pid %d ppid %d tid %d: Hello from main!\n", getpid(), getppid(), gettid());

	for(int i = 0; i < N; i++){
		err = pthread_create(&tid[i], NULL, mythread, &tid[i]);
		if (err) {
	    	printf("main: pthread_create() failed: %s\n", strerror(err));
			return -1;
		}
	}
	for (int i = 0; i < N; i++){
		if (pthread_join(tid[i], NULL) != 0){
			printf("Ошибка завершения потока\n");
		}
	}
	
//	sleep(60);
	printf("Программа завершилась дождавшись все потоки\n");
	return 0;
}

