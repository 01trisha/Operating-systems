#include "spinlock.h"
#include <sched.h>

void spinlock_init(spinlock_t *s) {
    atomic_store(&s->lock, 0);
}

void spinlock_lock(spinlock_t *s) {
    int expected;
    while (1) {
        expected = 0;
        if (atomic_compare_exchange_weak(&s->lock, &expected, 1)) {
            break;
        }
        sched_yield();
    }
}

void spinlock_unlock(spinlock_t *s) {
    atomic_store(&s->lock, 0);
}
