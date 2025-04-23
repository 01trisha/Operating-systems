#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define BLOCK_SIZE 4096

void ReverseString(char *string) {
    int length = strlen(string);
    for (int i = 0; i < length / 2; i++) {
        char temp = string[i];
        string[i] = string[length - i - 1]; //oello
        string[length - i - 1] = temp;//oellh
    } //olleh
}


void ReverseFileContent(const char *src_path, const char *dst_path) {
    int src_fd = open(src_path, O_RDONLY);
    if (src_fd == -1) {
        perror("open src");
        return;
    }

    int dst_fd = open(dst_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1) {
        perror("open dst");
        close(src_fd);
        return;
    }

    off_t size_file = lseek(src_fd, 0, SEEK_END);
    if (size_file == -1) {
        perror("lseek");
        close(src_fd);
        close(dst_fd);
        return;
    }

    char buffer[BLOCK_SIZE];
    off_t offset = size_file;

    while (offset > 0) {
        ssize_t to_read = (offset >= BLOCK_SIZE) ? BLOCK_SIZE : offset;
        offset -= to_read;

        if (lseek(src_fd, offset, SEEK_SET) == -1) {
            perror("lseek (loop)");
            break;
        }

        ssize_t bytes_read = read(src_fd, buffer, to_read);
        if (bytes_read != to_read) {
            perror("read");
            break;
        }

        // Реверсируем содержимое буфера
        for (ssize_t i = 0; i < to_read / 2; i++) {
            char temp = buffer[i];
            buffer[i] = buffer[to_read - i - 1];
            buffer[to_read - i - 1] = temp;
        }

        if (write(dst_fd, buffer, to_read) != to_read) {
            perror("write");
            break;
        }
    }

    close(src_fd);
    close(dst_fd);
}

void CopyReverseCatalog(const char* src_dir) {
    char dst_dir[256];
    if (strlen(src_dir) >= sizeof(dst_dir)) {
        fprintf(stderr, "Ошибка: путь к каталогу слишком длинный\n");
        return;
    }
    strcpy(dst_dir, src_dir); // Копируем путь к каталогу
    ReverseString(dst_dir); // Переворачиваем путь
    if (mkdir(dst_dir, 0755) == -1 && errno != EEXIST) { // создаем каталог
        perror("Ошибка при создании каталога");
        return;
    }
    DIR *dir = opendir(src_dir); // открываем каталог
    if (!dir) {
        perror("Ошибка при открытии каталога");
        return;
    }
    struct dirent *entry; // чтение записей из каталога
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // проверка на регулярный файл
            char src_file_path[512];
            char dst_file_path[512];
            snprintf(src_file_path, sizeof(src_file_path), "%s/%s", src_dir, entry->d_name); // src_dir - наша директория, наш файл из dentry name, src_file_path = "/путь/до/file.txt"
            snprintf(dst_file_path, sizeof(dst_file_path), "%s/", dst_dir); // dst_dir = "/home/user/documents/"
            ReverseString(entry->d_name); // entry->d_name = "file.txt", после ReverseString "txet.elif"
            strncat(dst_file_path, entry->d_name, sizeof(dst_file_path) - strlen(dst_file_path) - 1); // dst_file_path = "/home/user/backup/txet.elif"
            ReverseFileContent(src_file_path, dst_file_path);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    CopyReverseCatalog(argv[1]);
    return 0;
}

