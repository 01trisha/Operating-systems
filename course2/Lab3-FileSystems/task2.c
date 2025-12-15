#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

void create_dir(const char *path) {
    if (mkdir(path, 0777) == -1) perror("cannot create dir");
}

void read_dir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("cannot open dor for reading");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        printf("%s\n", entry->d_name);
    }

    closedir(dir);
}

void remove_dir(const char *path) {
    if (rmdir(path) == -1) perror("cannot rm dir");
}

void create_file(const char *path) {
    int fd = open(path, O_CREAT | O_WRONLY, 0666);
    if (fd == -1) perror("cannot open file for creating if file is already create");
    else close(fd);
}

void read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("cannot open file for reading");
        return;
    }

    char buf[1024];
    while (fgets(buf, sizeof(buf), f)) {
        fputs(buf, stdout);
    }

    fclose(f);
}

void remove_file(const char *path) {
    if (unlink(path) == -1) {
        perror("cannot remove file (unlink)");
    }
}

void create_symlink(const char *target, const char *linkname) {
    if (symlink(target, linkname) == -1) {
        perror("cannot create symlink");
    }
}

void read_symlink(const char *path) {
    char buf[256];
    ssize_t len = readlink(path, buf, sizeof(buf) - 1);
    if (len == -1) perror("cannot read link for reading symlink");
    else {
        buf[len] = '\0';
        printf("%s\n", buf);
    }
}

void read_symlink_target(const char *path) {
    char buf[256];
    ssize_t len = readlink(path, buf, sizeof(buf) - 1);
    if (len == -1) {
        perror("cannot read link fir reading target of symlink");
        return;
    }

    buf[len] = '\0';
    read_file(buf);
}

void remove_symlink(const char *path) {
    if (unlink(path) == -1) perror("cannot remove symlink(unlink sym)");
}

void create_hardlink(const char *target, const char *linkname) {
    if (link(target, linkname) == -1) {
        perror("cannot create hardlink");
    }
}

void remove_hardlink(const char *path) {
    if (unlink(path) == -1) perror("cannot remove hardlink (unlink (hardlink))");
}

void print_permissions_and_links(const char *path) {
    struct stat st;
    if (stat(path, &st) == -1) {
        perror("cannit get stat fir printing permissions and links");
        return;
    }

    printf("Permissions: %o\n", st.st_mode & 0777);
    printf("Hard links: %ld\n", (long)st.st_nlink);
}

void change_permissions(const char *path, const char *mode_str) {
    char *endptr;
    mode_t new_mode = strtol(mode_str, &endptr, 8);

    if (*endptr != '\0') {
        fprintf(stderr, "Invalid permission format: %s\n", mode_str);
        return;
    }

    if (chmod(path, new_mode) == -1) {
        perror("cannot change mod");
    }
}

void print_help() {
    printf("Available commands (use via symlink name or argv[0]):\n");
    printf("  create_dir <dir>             - Create a directory\n");
    printf("  read_dir <dir>               - List contents of a directory\n");
    printf("  remove_dir <dir>             - Remove an empty directory\n");
    printf("  create_file <file>           - Create an empty file\n");
    printf("  read_file <file>             - Read contents of a file\n");
    printf("  remove_file <file>           - Remove a file\n");
    printf("  create_symlink <target> <link> - Create a symbolic link\n");
    printf("  read_symlink <link>          - Read symbolic link target\n");
    printf("  read_symlink_target <link>   - Read contents of file linked by symlink\n");
    printf("  remove_symlink <link>        - Remove a symbolic link\n");
    printf("  create_hardlink <file> <link> - Create a hard link\n");
    printf("  remove_hardlink <link>       - Remove a hard link\n");
    printf("  print_info <file>            - Print file permissions and hard link count\n");
    printf("  change_permissions <file> <mode> - Change file permissions (e.g. 755)\n");
    printf("  help                         - Show this help message\n");
}


int main(int argc, char *argv[]) {
    if (argc < 2 && !strcmp(argv[0], "help") != 0) {
        fprintf(stderr, "Usage: %s <target>\n", argv[0]);
        return 1;
    }

    char *cmd = strrchr(argv[0], '/');
    cmd = cmd ? cmd + 1 : argv[0];

    if (strcmp(cmd, "create_dir") == 0) create_dir(argv[1]);
    else if (strcmp(cmd, "read_dir") == 0) read_dir(argv[1]);
    else if (strcmp(cmd, "remove_dir") == 0) remove_dir(argv[1]);
    else if (strcmp(cmd, "create_file") == 0) create_file(argv[1]);
    else if (strcmp(cmd, "read_file") == 0) read_file(argv[1]);
    else if (strcmp(cmd, "remove_file") == 0) remove_file(argv[1]);
    else if (strcmp(cmd, "read_symlink") == 0) read_symlink(argv[1]);
    else if (strcmp(cmd, "read_symlink_target") == 0) read_symlink_target(argv[1]);
    else if (strcmp(cmd, "remove_symlink") == 0) remove_symlink(argv[1]);
    else if (strcmp(cmd, "create_hardlink") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s <source> <hardlink_name>\n", argv[0]);
            return 1;
        }
        create_hardlink(argv[1], argv[2]);
    }
    else if (strcmp(cmd, "create_symlink") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s <source> <symlink_name>\n", argv[0]);
            return 1;
        }
        create_symlink(argv[1], argv[2]);
    }
    else if (strcmp(cmd, "remove_hardlink") == 0) remove_hardlink(argv[1]);
    else if (strcmp(cmd, "print_info") == 0) print_permissions_and_links(argv[1]);
    else if (strcmp(cmd, "change_permissions") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s <file> <mode>\n", argv[0]);
            return 1;
        }
        change_permissions(argv[1], argv[2]);
    }
    else if (strcmp(cmd, "help") == 0) print_help();
    else fprintf(stderr, "Unknown command: %s\n", cmd);

    return 0;
}

