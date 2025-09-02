#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int global_var = 22;

int main() {
    int local_var = 203;
    
    printf("Дед PID: %d\n", getpid());
    printf("Дед: глобальная %d по адресу %p\n", global_var, &global_var);
    printf("Дед: локальная %d по адресу %p\n", local_var, &local_var);
    
    // Создаём отца
    pid_t father_pid = fork();
    
    if (father_pid == -1) {
        perror("Ошибка fork");
        return 1;
    } else if (father_pid == 0) {
        // Код отца
        printf("\nОтец PID: %d (сын %d)\n", getpid(), getppid());
        
        // Создаём сына
        pid_t son_pid = fork();
        
        if (son_pid == -1) {
            perror("Ошибка fork");
            exit(1);
        } else if (son_pid == 0) {
            // Код сына
            printf("\nСын PID: %d (сын %d)\n", getpid(), getppid());
            printf("Сын спит 200 секунд...\n");
            sleep(200);  // Сын спит 200 секунд
            printf("Сын завершается\n");
            exit(0);
        } else {
            // Отец живёт 15 секунд и завершается
            printf("Отец живёт 15 секунд...\n");
            sleep(15);
            printf("Отец завершается (станет зомби)\n");
            exit(0);
        }
    } else {
        // Код деда
        printf("Дед спит 200 секунд...\n");
        sleep(200);  // Дед спит 200 секунд
        printf("Дед завершается\n");
    }
    
    return 0;
}
