#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    printf("RUID: %d\n", getuid());
    printf("EUID: %d\n", geteuid());
    
    FILE *file = fopen("onlyroot.txt", "r");
    if (file == NULL) {
        perror("Ошибка при открытии файла");
        return 1;
    }
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("Содержимое файла: %s", buffer);
    }
    
    fclose(file);
    return 0;
}
