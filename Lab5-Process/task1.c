#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>

int global_var = 22; // Глобальная переменная

int main() {
    int local_var = 203; // Локальная переменная
    
    // Вывод информации в родительском процессе
    printf("P_proc: global var %d and its address %p \n", global_var, &global_var);
    printf("P_proc: local var %d and its address %p \n", local_var, &local_var);
    pid_t pid_parent = getpid();
    printf("My pid: %d \n", pid_parent);
    printf("cat /proc/%d/maps\n", pid_parent);
    
    // Создание дочернего процесса
    pid_t pid_fork = fork();
    sleep(10);
    
    if (pid_fork == -1) {
        perror("Fork");
        return 1;
    } else if (pid_fork == 0) {
        // Дочерний процесс
        printf("Child pid : %d \n", getpid());
        printf("Parent pid: %d \n", getppid());
        printf("C_proc: global var %d and its address %p \n", global_var, &global_var);
        printf("C_proc: local var %d and its address %p \n", local_var, &local_var);
	printf("cat /proc/%d/maps\n", getpid());
        
        // Изменение переменных в дочернем процессе
	sleep(10);
        global_var = 20;
        local_var = 207;
        printf("C_proc: global var %d and its address %p \n", global_var, &global_var);
        printf("C_proc: local var %d and its address %p \n", local_var, &local_var);
        exit(2);
    } else {
        // Родительский процесс
        printf("P_proc: global var %d and its address %p \n", global_var, &global_var);
        printf("P_proc: local var %d and its address %p \n", local_var, &local_var);
        sleep(30);
        
        // Ожидание завершения дочернего процесса
        int status;
        pid_t child = wait(&status);
        if (WIFEXITED(status)) {
            printf("Child process %d ended with %d status\n", child, WEXITSTATUS(status));
        } else {
            printf("Child process %d ended without ok. Status: %d \n", child, WIFEXITED(status));
        }
    }
    return 0;
}
