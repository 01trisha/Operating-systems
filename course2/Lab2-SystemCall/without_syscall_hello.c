#include <stdint.h>

// Системные вызовы для AArch64
#define SYS_WRITE 64   // Номер системного вызова write
#define SYS_EXIT  93   // Номер системного вызова exit

// Функция для выполнения системного вызова
static inline long syscall(long n, long arg1, long arg2, long arg3) {
    register long x8 __asm__("x8") = n;   // Номер системного вызова
    register long x0 __asm__("x0") = arg1; // Аргумент 1
    register long x1 __asm__("x1") = arg2; // Аргумент 2
    register long x2 __asm__("x2") = arg3; // Аргумент 3
    __asm__ __volatile__(
        "svc #0"  // Выполняем системный вызов
        : "+r"(x0)
        : "r"(x8), "r"(x0), "r"(x1), "r"(x2)
        : "memory"
    );
    return x0;  // Возвращаем результат
}

int main() {
    const char msg[] = "Hello world\n";
    const uint64_t len = sizeof(msg) - 1;  // Длина строки без нулевого байта

    // Системный вызов write(1, msg, len)
    syscall(SYS_WRITE, 1, (long)msg, len);

    // Системный вызов exit(0)
    syscall(SYS_EXIT, 0, 0, 0);

    return 0;  // Этот код никогда не выполнится
}
