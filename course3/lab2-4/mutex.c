#define _GNU_SOURCE
#include "mutex.h"
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>

static int futex_wait(atomic_int *futex, int val) {
    return syscall(SYS_futex, futex, FUTEX_WAIT, val, NULL, NULL, 0);
}

static int futex_wake(atomic_int *futex, int n) {
    return syscall(SYS_futex, futex, FUTEX_WAKE, n, NULL, NULL, 0);
}

void mutex_init(mutex_t *m) {
    atomic_store(&m->futex, 0);
}

void mutex_lock(mutex_t *m) {
    int expected = 0;
    if (atomic_compare_exchange_strong(&m->futex, &expected, 1)) {
        return;
    }
    
    while (1) {
        if (expected == 2 || atomic_compare_exchange_strong(&m->futex, &expected, 2)) {
            futex_wait(&m->futex, 2);
        }
        
        expected = 0;
        if (atomic_compare_exchange_strong(&m->futex, &expected, 2)) {
            return;
        }
    }
}

void mutex_unlock(mutex_t *m) {
    int old = atomic_exchange(&m->futex, 0);
    
    if (old == 2) {
        futex_wake(&m->futex, 1);
    }
}
