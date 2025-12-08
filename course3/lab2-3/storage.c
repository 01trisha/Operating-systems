#include "storage.h"

static const char *sample_strings[] = {
    "a", "ab", "abc", "abcd", "abcde",
    "short", "medium length", "a bit longer string here",
    "x", "xy", "xyz", "test", "hello", "world",
    "programming", "operating systems", "synchronization"
};
static const int num_samples = sizeof(sample_strings) / sizeof(sample_strings[0]);

Storage* storage_init(int size, int sync_type) {
    Storage *s = malloc(sizeof(Storage));
    if (!s) {
        fprintf(stderr, "Cannot allocate memory for storage\n");
        abort();
    }
    
    s->first = NULL;
    s->size = size;
    s->sync_type = sync_type;
    
    // Initialize storage lock
    if (sync_type == SYNC_MUTEX) {
        pthread_mutex_init(&s->storage_lock.storage_mutex, NULL);
    } else if (sync_type == SYNC_SPINLOCK) {
        pthread_spin_init(&s->storage_lock.storage_spinlock, PTHREAD_PROCESS_PRIVATE);
    } else if (sync_type == SYNC_RWLOCK) {
        pthread_rwlock_init(&s->storage_lock.storage_rwlock, NULL);
    }
    
    srand(time(NULL));
    
    Node *prev = NULL;
    for (int i = 0; i < size; i++) {
        Node *new_node = malloc(sizeof(Node));
        if (!new_node) {
            fprintf(stderr, "Cannot allocate memory for node\n");
            abort();
        }
        
        // Fill with random string
        const char *str = sample_strings[rand() % num_samples];
        strncpy(new_node->value, str, 99);
        new_node->value[99] = '\0';
        new_node->next = NULL;
        
        // Initialize synchronization primitive
        if (sync_type == SYNC_MUTEX) {
            pthread_mutex_init(&new_node->sync.mutex, NULL);
        } else if (sync_type == SYNC_SPINLOCK) {
            pthread_spin_init(&new_node->sync.spinlock, PTHREAD_PROCESS_PRIVATE);
        } else if (sync_type == SYNC_RWLOCK) {
            pthread_rwlock_init(&new_node->sync.rwlock, NULL);
        }
        
        if (prev == NULL) {
            s->first = new_node;
        } else {
            prev->next = new_node;
        }
        prev = new_node;
    }
    
    return s;
}

void storage_destroy(Storage *s) {
    if (!s) return;
    
    Node *current = s->first;
    while (current) {
        Node *tmp = current;
        current = current->next;
        
        if (s->sync_type == SYNC_MUTEX) {
            pthread_mutex_destroy(&tmp->sync.mutex);
        } else if (s->sync_type == SYNC_SPINLOCK) {
            pthread_spin_destroy(&tmp->sync.spinlock);
        } else if (s->sync_type == SYNC_RWLOCK) {
            pthread_rwlock_destroy(&tmp->sync.rwlock);
        }
        
        free(tmp);
    }
    
    // Destroy storage lock
    if (s->sync_type == SYNC_MUTEX) {
        pthread_mutex_destroy(&s->storage_lock.storage_mutex);
    } else if (s->sync_type == SYNC_SPINLOCK) {
        pthread_spin_destroy(&s->storage_lock.storage_spinlock);
    } else if (s->sync_type == SYNC_RWLOCK) {
        pthread_rwlock_destroy(&s->storage_lock.storage_rwlock);
    }
    
    free(s);
}

void storage_print_stats(Storage *s) {
    printf("Storage: size=%d, sync_type=%d\n", s->size, s->sync_type);
}
