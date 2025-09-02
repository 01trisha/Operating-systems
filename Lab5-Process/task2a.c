#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int global_var = 22;

int main() {
    int local_var = 203;
    printf("P_proc: global var %d and its address %p \n", global_var, &global_var);
    printf("P_proc: local var %d and its address %p \n", local_var, &local_var);
    pid_t pid_parent = getpid();
    printf("My pid: %d \n", pid_parent);
    printf("cat /proc/%d/maps\n", pid_parent);
    pid_t pid_fork = fork();
    if (pid_fork == -1) {
        perror("Fork");
        return 1;
    } else if (pid_fork == 0) {
        // Дочерний процесс
        printf("Child pid : %d \n", getpid());
	printf("ps aux | grep Z\n");
        printf("Parent pid: %d \n", getppid());
        printf("C_proc: global var %d and its address %p \n", global_var, &global_var);
        printf("C_proc: local var %d and its address %p \n", local_var, &local_var);
        
        global_var = 20;
        local_var = 207;
        printf("C_proc: global var %d and its address %p \n", global_var, &global_var);
        printf("C_proc: local var %d and its address %p \n", local_var, &local_var);
        
        //sleep(200);
        exit(2);
    } else {
        // Родительский процесс
        printf("P_proc: global var %d and its address %p \n", global_var, &global_var);
        printf("P_proc: local var %d and its address %p \n", local_var, &local_var);
	printf("a");
        sleep(100);
        printf("Finish parent process \n");
    }
    
    printf("P_proc: global var %d and its address %p \n", global_var, &global_var);
    return 0;
}
