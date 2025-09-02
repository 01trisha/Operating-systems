#include <stdio.h>
#include <unistd.h>

int main() {
    printf("My pid is %d \n", getpid());
    sleep(1);
    execvp("./task2-1", NULL);
    perror("execvp");
    printf("Hello world \n"); //helloworld выводится только в случае ошибки
    return 0;
}
