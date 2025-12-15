#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

//глобальные переменные разных типов
int global_int = 10;//инициализированная глобальная
int global_var;// неинициализированная глобальная
const int global_const = 2;// глобальная константа

void print_locals() {
    printf("\n=== Локальные переменные в функции ===\n");
    int local_a;
    long long local_b;
    char local_c;
    float local_d;
    printf("Локальная int 'a' и её адрес: %p\n", (void *)&local_a);
    printf("Локальная long long 'b' и её адрес: %p\n", (void *)&local_b);
    printf("Локальная char 'c' и её адрес: %p\n", (void *)&local_c);
    printf("Локальная float 'd' и её адрес: %p\n", (void *)&local_d);
}

void print_statics() {
    printf("\n=== Статические переменные в функции ===\n");
    static int static_a;
    static long long static_b;
    static char static_c;
    static float static_d;
    printf("Статическая int 'a' и её адрес: %p\n", (void *)&static_a);
    printf("Статическая long long 'b' и её адрес: %p\n", (void *)&static_b);
    printf("Статическая char 'c' и её адрес: %p\n", (void *)&static_c);
    printf("Статическая float 'd' и её адрес: %p\n", (void *)&static_d);
}

void print_constants() {
    printf("\n=== Константные переменные в функции ===\n");
    const int const_a;
    const long long const_b;
    const char const_c;
    const float const_d;
    printf("Константная int 'a' и её адрес: %p\n", (void *)&const_a);
    printf("Константная long long 'b' и её адрес: %p\n", (void *)&const_b);
    printf("Константная char 'c' и её адрес: %p\n", (void *)&const_c);
    printf("Константная float 'd' и её адрес: %p\n", (void *)&const_d);
}

void print_globals() {
    printf("\n=== Глобальные переменные ===\n");
    printf("Инициализированная глобальная 'global_int' и её адрес: %p\n", (void *)&global_int);
    printf("Неинициализированная глобальная 'global_var' и её адрес: %p\n", (void *)&global_var);
    printf("Глобальная константа 'global_const' и её адрес: %p\n", (void *)&global_const);
}

int *variable_address() {
    int a = 10;
    printf("\nАдрес локальной переменной 'a' внутри функции: %p\n", (void *)&a);
    return &a;  //oпасное действие - возврат адреса локальной переменной
}

void buffer_experiments() {
    printf("\n=== Эксперименты с динамической памятью ===\n");
    
    //выделение и использование буфера
    char *buffer = malloc(100);
    assert(buffer != NULL);
    strcpy(buffer, "qwertyuiop");
    printf("Содержимое первого буфера до освобождения: %s\n", buffer);
    
    //oсвобождение и проверка
    free(buffer);
    printf("Содержимое первого буфера после освобождения: %s (неопределённое поведение!)\n", buffer);
    
    //новый буфер
    char *new_buffer = malloc(100);
    strcpy(new_buffer, "qwertyuiopqwertyuiop");
    printf("Содержимое нового буфера до освобождения: %s\n", new_buffer);
    
    //работа с серединой буфера
    char *middle = new_buffer + 50;
    printf("Содержимое с середины нового буфера: %s\n", middle);
    
    //oпасное освобождение середины!
    printf("Попытка освободить середину буфера\n");
    free(middle);  //можно освобождать только начало выделенного блока
    printf("Содержимое буфера после ошибочного освобождения: %s (возможен крах!)\n", new_buffer);
    
    //правильное освобождение
    free(new_buffer);
}

void env_variable_demo() {
    printf("\n=== Работа с переменными окружения ===\n");
    
    //чтение переменной
    printf("Текущее значение MY_VAR: %s\n", getenv("MY_VAR"));
    
    //изменение (только для текущего процесса)
    setenv("MY_VAR", "ytrewq", 1);
    printf("Новое значение MY_VAR: %s\n", getenv("MY_VAR"));
    
    printf("\nЗамечание: это изменение только для текущего процесса.\n");
    printf("Чтобы проверить, выполните в другом терминале:\n");
    printf("echo $MY_VAR\n");
}

int main() {
    printf("=== Исследование адресного пространства процесса ===\n");
    printf("PID этого процесса: %d\n", getpid());
    printf("\n/proc/%d/maps\n", getpid());
    
    // 1. Вывод адресов разных переменных
    print_locals();
    print_statics();
    print_constants();
    print_globals();
    
    printf("\nПауза 10 секунд для анализа /proc/%d/maps...\n", getpid());
    sleep(10);
    
    демонстрация проблемы с возвратом адреса локальной переменной
    printf("\n=== Возврат адреса локальной переменной ===\n");
    int *danger_ptr = variable_address();
    printf("Полученный адрес в main: %p\n", (void *)danger_ptr);
    printf("Значение по этому адресу: %d (неопределённое поведение)\n", *danger_ptr);
    
    printf("\nПауза 10 секунд\n");
    sleep(10);
    
    //эксперименты с кучей
    buffer_experiments();
    
    printf("\nпауза 10 секунд\n");
    sleep(10);
    
    //работа с переменными окружения
    env_variable_demo();
    
    return 0;
}
