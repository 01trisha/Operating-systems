#include "storage.h"
#include <signal.h>
#include <unistd.h>

// Global counters
volatile long ascending_iterations = 0;
volatile long descending_iterations = 0;
volatile long equal_iterations = 0;
volatile long swap_count_1 = 0;
volatile long swap_count_2 = 0;
volatile long swap_count_3 = 0;
volatile int running = 1;

// Lock/unlock helpers
static inline void lock_node(Node *node, int sync_type) {
    if (sync_type == SYNC_MUTEX) {
        pthread_mutex_lock(&node->sync.mutex);
    } else if (sync_type == SYNC_SPINLOCK) {
        pthread_spin_lock(&node->sync.spinlock);
    } else if (sync_type == SYNC_RWLOCK) {
        pthread_rwlock_wrlock(&node->sync.rwlock);
    }
}

static inline void unlock_node(Node *node, int sync_type) {
    if (sync_type == SYNC_MUTEX) {
        pthread_mutex_unlock(&node->sync.mutex);
    } else if (sync_type == SYNC_SPINLOCK) {
        pthread_spin_unlock(&node->sync.spinlock);
    } else if (sync_type == SYNC_RWLOCK) {
        pthread_rwlock_unlock(&node->sync.rwlock);
    }
}

static inline void rdlock_node(Node *node, int sync_type) {
    if (sync_type == SYNC_MUTEX) {
        pthread_mutex_lock(&node->sync.mutex);
    } else if (sync_type == SYNC_SPINLOCK) {
        pthread_spin_lock(&node->sync.spinlock);
    } else if (sync_type == SYNC_RWLOCK) {
        pthread_rwlock_rdlock(&node->sync.rwlock);
    }
}

// Thread 1: Count ascending pairs
void* count_ascending(void *arg) {
    Storage *s = (Storage *)arg;
    
    while (running) {
        int count = 0;
        Node *current = s->first;
        
        if (!current) {
            ascending_iterations++;
            continue;
        }
        
        rdlock_node(current, s->sync_type);
        int len1 = strlen(current->value);
        
        while (current->next) {
            Node *next = current->next;
            rdlock_node(next, s->sync_type);
            int len2 = strlen(next->value);
            
            if (len1 < len2) {
                count++;
            }
            
            unlock_node(current, s->sync_type);
            current = next;
            len1 = len2;
        }
        
        unlock_node(current, s->sync_type);
        ascending_iterations++;
    }
    
    return NULL;
}

// Thread 2: Count descending pairs
void* count_descending(void *arg) {
    Storage *s = (Storage *)arg;
    
    while (running) {
        int count = 0;
        Node *current = s->first;
        
        if (!current) {
            descending_iterations++;
            continue;
        }
        
        rdlock_node(current, s->sync_type);
        int len1 = strlen(current->value);
        
        while (current->next) {
            Node *next = current->next;
            rdlock_node(next, s->sync_type);
            int len2 = strlen(next->value);
            
            if (len1 > len2) {
                count++;
            }
            
            unlock_node(current, s->sync_type);
            current = next;
            len1 = len2;
        }
        
        unlock_node(current, s->sync_type);
        descending_iterations++;
    }
    
    return NULL;
}

// Thread 3: Count equal pairs
void* count_equal(void *arg) {
    Storage *s = (Storage *)arg;
    
    while (running) {
        int count = 0;
        Node *current = s->first;
        
        if (!current) {
            equal_iterations++;
            continue;
        }
        
        rdlock_node(current, s->sync_type);
        int len1 = strlen(current->value);
        
        while (current->next) {
            Node *next = current->next;
            rdlock_node(next, s->sync_type);
            int len2 = strlen(next->value);
            
            if (len1 == len2) {
                count++;
            }
            
            unlock_node(current, s->sync_type);
            current = next;
            len1 = len2;
        }
        
        unlock_node(current, s->sync_type);
        equal_iterations++;
    }
    
    return NULL;
}

// Swap thread helper - swaps nodes at random positions
void* swap_thread(void *arg) {
    void **args = (void **)arg;
    Storage *s = (Storage *)args[0];
    volatile long *counter = (volatile long *)args[1];
    
    while (running) {
        // Pick random position to potentially swap
        if (s->size < 2) continue;
        
        int pos = rand() % (s->size - 1);
        
        // Navigate to position without locking
        Node *prev = NULL;
        Node *current = s->first;
        
        for (int i = 0; i < pos && current; i++) {
            prev = current;
            current = current->next;
        }
        
        if (!current || !current->next) {
            continue;
        }
        
        Node *next = current->next;
        
        // Lock nodes in order from head to tail to avoid deadlock
        // Lock prev (if exists), then current, then next
        if (prev) {
            lock_node(prev, s->sync_type);
            // Check if still valid after locking
            if (prev->next != current) {
                unlock_node(prev, s->sync_type);
                continue;
            }
        }
        
        lock_node(current, s->sync_type);
        // Check if still valid after locking
        if (current->next != next) {
            unlock_node(current, s->sync_type);
            if (prev) unlock_node(prev, s->sync_type);
            continue;
        }
        
        lock_node(next, s->sync_type);
        
        // Decide randomly whether to swap
        if (rand() % 2 == 0) {
            Node *next_next = next->next;
            
            // Perform swap: current and next exchange positions
            if (prev) {
                prev->next = next;
            } else {
                s->first = next;
            }
            next->next = current;
            current->next = next_next;
            
            (*counter)++;
        }
        
        // Unlock in reverse order
        unlock_node(next, s->sync_type);
        unlock_node(current, s->sync_type);
        if (prev) {
            unlock_node(prev, s->sync_type);
        }
    }
    
    free(args);
    return NULL;
}

void signal_handler(int sig) {
    running = 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <list_size> <sync_type>\n", argv[0]);
        fprintf(stderr, "  sync_type: 0=mutex, 1=spinlock, 2=rwlock\n");
        return 1;
    }
    
    int size = atoi(argv[1]);
    int sync_type = atoi(argv[2]);
    
    if (size <= 0 || sync_type < 0 || sync_type > 2) {
        fprintf(stderr, "Invalid parameters\n");
        return 1;
    }
    
    signal(SIGINT, signal_handler);
    
    printf("Initializing storage with size=%d, sync_type=%d\n", size, sync_type);
    Storage *storage = storage_init(size, sync_type);
    
    pthread_t readers[3];
    pthread_t swappers[3];
    
    // Start reader threads
    pthread_create(&readers[0], NULL, count_ascending, storage);
    pthread_create(&readers[1], NULL, count_descending, storage);
    pthread_create(&readers[2], NULL, count_equal, storage);
    
    // Start swapper threads
    void **args1 = malloc(2 * sizeof(void *));
    args1[0] = storage;
    args1[1] = (void *)&swap_count_1;
    pthread_create(&swappers[0], NULL, swap_thread, args1);
    
    void **args2 = malloc(2 * sizeof(void *));
    args2[0] = storage;
    args2[1] = (void *)&swap_count_2;
    pthread_create(&swappers[1], NULL, swap_thread, args2);
    
    void **args3 = malloc(2 * sizeof(void *));
    args3[0] = storage;
    args3[1] = (void *)&swap_count_3;
    pthread_create(&swappers[2], NULL, swap_thread, args3);
    
    // Monitor thread
    const char *sync_names[] = {"mutex", "spinlock", "rwlock"};
    printf("Running with %s... Press Ctrl+C to stop.\n", sync_names[sync_type]);
    
    while (running) {
        sleep(1);
        printf("Iterations: asc=%ld, desc=%ld, eq=%ld | Swaps: t1=%ld, t2=%ld, t3=%ld\n",
               ascending_iterations, descending_iterations, equal_iterations,
               swap_count_1, swap_count_2, swap_count_3);
    }
    
    printf("\nStopping threads...\n");
    
    // Wait for threads
    for (int i = 0; i < 3; i++) {
        pthread_join(readers[i], NULL);
        pthread_join(swappers[i], NULL);
    }
    
    printf("\nFinal statistics:\n");
    printf("Ascending iterations: %ld\n", ascending_iterations);
    printf("Descending iterations: %ld\n", descending_iterations);
    printf("Equal iterations: %ld\n", equal_iterations);
    printf("Total swaps: %ld (t1=%ld, t2=%ld, t3=%ld)\n",
           swap_count_1 + swap_count_2 + swap_count_3,
           swap_count_1, swap_count_2, swap_count_3);
    
    storage_destroy(storage);
    
    return 0;
}
