#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <limits.h>

#define PAGE_SIZE sysconf(_SC_PAGESIZE)

// обработчик sigsegv
void handle_sigsegv() {
    write(0, "получен sigsegv сигнал.", 24);
    exit(1);
}

// рекурсивная функция для демонстрации роста стека
void resursive_stack_array() {
    char array[4096]; // выделяем 4кб на стеке
    printf("указатель на массив: %p \n", (void *) &array);
    sleep(1);
    resursive_stack_array(); // бесконечная рекурсия
}

// бесконечный цикл с выделением/освобождением кучи
_Noreturn void heap_array_while() {
    while (true) {
        char *array = malloc(1*1024*1024); // выделяем очень большой блок
        printf("указатель на массив: %p \n", (void *) &array);
        sleep(1);
        free(array); // освобождаем
    }
}

// создание и удаление региона памяти
void create_add() {

	printf("проверьте maps до создания региона\n");
	printf("cat /proc/");

	sleep(10);
    size_t region_size = 10 * PAGE_SIZE; // 10 страниц
    printf("создаю новый регион памяти\n");
    void *region_addr = mmap(
    NULL,                   // адрес (NULL = "пусть ОС сама выберет")
    10 * PAGE_SIZE,         // размер (10 страниц)
    PROT_READ | PROT_WRITE, // права: чтение + запись
    MAP_PRIVATE | MAP_ANONYMOUS, // флаги: приватная + анонимная (не файл)
    -1,                     // файловый дескриптор (не используется)
    0                       // смещение (не используется)
);
    if (region_addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    printf("создан регион по адресу %p размером %zu байт\n", region_addr, region_size);

    sleep(10);
    printf("удаляю регион\n");
    if (munmap(region_addr, region_size) == -1) {
        perror("munmap");
        exit(1);
    }

    printf("проверьте опять мапс процесса");
    sleep(10);
}

// изменение прав доступа к региону (без обработки ошибок)
void create_add_change() {
    size_t region_size = 10 * PAGE_SIZE;
    void *region_addr = mmap(NULL, region_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region_addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    printf("создан регион по адресу %p размером %zu байт\n", region_addr, region_size);

    char *message = "1234567890";
    printf("пишу в регион: %s\n", message);
    sprintf(region_addr, "%s", message);
    printf("читаю из региона: %s\n", (char *) region_addr);
    
    printf("меняю права доступа\n");
    if (mprotect(region_addr, region_size, PROT_WRITE) == -1) {
        perror("mprotect");
        exit(1);
    }
    
    printf("пытаюсь читать после mprotect(PROT_WRITE):\n");
    printf("%s\n", (char *) region_addr);

    if (mprotect(region_addr, region_size, PROT_READ) == -1) {
        perror("mprotect");
        exit(1);
    }

    printf("пытаюсь писать после mprotect(PROT_READ):\n");
    sprintf(region_addr, "%s", message); // здесь будет segfault
    if (munmap(region_addr, region_size) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }
}

// то же самое, но с обработкой sigsegv
void create_add_change_handled() {
    size_t region_size = 10 * PAGE_SIZE;
    void *region_addr = mmap(NULL, region_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region_addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    printf("создан регион по адресу %p размером %zu байт\n", region_addr, region_size);

    char *message = "привет мир, мне нравится изучать ос!";
    printf("пишу в регион: %s\n", message);
    sprintf(region_addr, "%s", message);
    printf("читаю из региона: %s\n", (char *) region_addr);

    printf("меняю права доступа\n");
    if (mprotect(region_addr, region_size, PROT_WRITE) == -1) {
        perror("mprotect");
        exit(1);
    }
    signal(SIGSEGV, handle_sigsegv); // устанавливаем обработчик

    printf("пытаюсь читать после mprotect(PROT_WRITE):\n");
    printf("%s\n", (char *) region_addr);

    if (mprotect(region_addr, region_size, PROT_READ) == -1) {
        perror("mprotect");
        exit(1);
    }

    printf("пытаюсь писать после mprotect(PROT_READ):\n");
    sprintf(region_addr, "%s", message); // здесь будет segfault, но перехваченный
    if (munmap(region_addr, region_size) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }
}

// частичное удаление региона
void create_add_part() {
    size_t region_size = 10 * PAGE_SIZE;
    printf("создаю новый регион памяти\n");
    void *region_addr = mmap(NULL, region_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region_addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    printf("создан регион по адресу %p размером %zu байт\n", region_addr, region_size);
    sleep(10);

    size_t partial_size = 3 * PAGE_SIZE;
    void *partial_addr = region_addr + 3 * PAGE_SIZE;
    printf("удаляю часть региона по адресу %p размером %zu байт\n", partial_addr, partial_size);
    if (munmap(partial_addr, partial_size) == -1) {
        perror("munmap");
        exit(1);
    }
    sleep(10);

    printf("удаляю оставшуюся часть региона\n");
    if (munmap(region_addr, region_size) == -1) {
        perror("munmap");
        exit(1);
    }
    sleep(10);
}

int main() {
    printf("мой pid: %d \n", getpid());
    printf("cat /proc/%d/maps\n", getpid());


    sleep(2);
    //resursive_stack_array(); //тест стека
    //heap_array_while(); //тест кучи
    //create_add(); // простой mmap
    //create_add_change(); //тест прав доступа
    //create_add_change_handled(); //тест прав с обработчиком
//	sleep(10);
    create_add_part(); //частичное удаление
    return 0;
}
