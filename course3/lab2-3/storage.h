#ifndef __STORAGE_H__
#define __STORAGE_H__

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define SYNC_MUTEX 0
#define SYNC_SPINLOCK 1
#define SYNC_RWLOCK 2

typedef struct _Node {
    char value[100];
    struct _Node* next;
    union {
        pthread_mutex_t mutex;
        pthread_spinlock_t spinlock;
        pthread_rwlock_t rwlock;
    } sync;
} Node;

typedef struct _Storage {
    Node *first;
    int size;
    int sync_type;
    union {
        pthread_mutex_t storage_mutex;
        pthread_spinlock_t storage_spinlock;
        pthread_rwlock_t storage_rwlock;
    } storage_lock;
} Storage;

Storage* storage_init(int size, int sync_type);
void storage_destroy(Storage *s);
void storage_print_stats(Storage *s);

#endif // __STORAGE_H__
