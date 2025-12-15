#include "queue.h"

#define STORAGE_CAPACITY 1000
#define THREAD_COUNT 6
#define ASC 0
#define DESC 1
#define EQ 2
#define SWAP1 3
#define SWAP2 4
#define SWAP3 5

volatile int running = 1;
int asc_nodes_read = 0, desc_nodes_read = 0, eq_nodes_read = 0;

void *ascending_thread(void *data) {
    ThreadData *td = (ThreadData *)data;
    Storage *storage = td->storage;
    int *counter = td->counter;
    
    while (running) {
        int local_count = 0;
        int nodes_read = 0;
        
        pthread_mutex_lock(&storage->head_mutex);
        Node *curr = storage->first;
        if (curr == NULL) {
            pthread_mutex_unlock(&storage->head_mutex);
            usleep(1000);
            continue;
        }
        pthread_mutex_lock(&curr->sync);
        pthread_mutex_unlock(&storage->head_mutex);
        
        Node *next = curr->next;
        nodes_read = 1;
        
        while (next != NULL && running) {
            pthread_mutex_lock(&next->sync);
            nodes_read++;
            
            if (strlen(curr->value) < strlen(next->value)) {
                local_count++;
            }
            
            pthread_mutex_unlock(&curr->sync);
            curr = next;
            next = curr->next;
        }
        pthread_mutex_unlock(&curr->sync);
        
        pthread_mutex_lock(&global_mutex);
        asc_pairs = local_count;
        asc_nodes_read = nodes_read;
        (*counter)++;
        pthread_mutex_unlock(&global_mutex);
        
        usleep(1000);
    }
    return NULL;
}

void *descending_thread(void *data) {
    ThreadData *td = (ThreadData *)data;
    Storage *storage = td->storage;
    int *counter = td->counter;
    
    while (running) {
        int local_count = 0;
        int nodes_read = 0;
        
        pthread_mutex_lock(&storage->head_mutex);
        Node *curr = storage->first;
        if (curr == NULL) {
            pthread_mutex_unlock(&storage->head_mutex);
            usleep(1000);
            continue;
        }
        pthread_mutex_lock(&curr->sync);
        pthread_mutex_unlock(&storage->head_mutex);
        
        Node *next = curr->next;
        nodes_read = 1;
        
        while (next != NULL && running) {
            pthread_mutex_lock(&next->sync);
            nodes_read++;
            
            if (strlen(curr->value) > strlen(next->value)) {
                local_count++;
            }
            
            pthread_mutex_unlock(&curr->sync);
            curr = next;
            next = curr->next;
        }
        pthread_mutex_unlock(&curr->sync);
        
        pthread_mutex_lock(&global_mutex);
        desc_pairs = local_count;
        desc_nodes_read = nodes_read;
        (*counter)++;
        pthread_mutex_unlock(&global_mutex);
        
        usleep(1000);
    }
    return NULL;
}

void *equal_length_thread(void *data) {
    ThreadData *td = (ThreadData *)data;
    Storage *storage = td->storage;
    int *counter = td->counter;
    
    while (running) {
        int local_count = 0;
        int nodes_read = 0;
        
        pthread_mutex_lock(&storage->head_mutex);
        Node *curr = storage->first;
        if (curr == NULL) {
            pthread_mutex_unlock(&storage->head_mutex);
            usleep(1000);
            continue;
        }
        pthread_mutex_lock(&curr->sync);
        pthread_mutex_unlock(&storage->head_mutex);
        
        Node *next = curr->next;
        nodes_read = 1;
        
        while (next != NULL && running) {
            pthread_mutex_lock(&next->sync);
            nodes_read++;
            
            if (strlen(curr->value) == strlen(next->value)) {
                local_count++;
            }
            
            pthread_mutex_unlock(&curr->sync);
            curr = next;
            next = curr->next;
        }
        pthread_mutex_unlock(&curr->sync);
        
        pthread_mutex_lock(&global_mutex);
        eq_pairs = local_count;
        eq_nodes_read = nodes_read;
        (*counter)++;
        pthread_mutex_unlock(&global_mutex);
        
        usleep(1000);
    }
    return NULL;
}

void *swap_thread(void *data) {
    ThreadData *td = (ThreadData *)data;
    Storage *storage = td->storage;
    int *counter = td->counter;
    int thread_id = td->thread_id - SWAP1;
    
    while (running) {
        pthread_mutex_lock(&global_mutex);
        (*counter)++;
        pthread_mutex_unlock(&global_mutex);
        
        int list_length = get_list_length(storage);
        if (list_length < 3) {
            usleep(1000);
            continue;
        }
        
        int pos = rand() % (list_length - 2);
        
        pthread_mutex_lock(&storage->head_mutex);
        Node *first = storage->first;
        if (first == NULL) {
            pthread_mutex_unlock(&storage->head_mutex);
            usleep(1000);
            continue;
        }
        pthread_mutex_lock(&first->sync);
        pthread_mutex_unlock(&storage->head_mutex);
        
        Node *prev = NULL;
        Node *curr = first;
        
        for (int i = 0; i < pos && curr != NULL; i++) {
            Node *next = curr->next;
            if (next == NULL) break;
            pthread_mutex_lock(&next->sync);
            if (prev) pthread_mutex_unlock(&prev->sync);
            prev = curr;
            curr = next;
        }
        
        if (curr == NULL || curr->next == NULL) {
            if (prev) pthread_mutex_unlock(&prev->sync);
            pthread_mutex_unlock(&curr->sync);
            usleep(1000);
            continue;
        }
        
        Node *A = curr;
        Node *B = A->next;
        pthread_mutex_lock(&B->sync);
        
        int should_swap = rand() % 2;
        
        if (should_swap) {
            if (prev == NULL) {
                pthread_mutex_lock(&storage->head_mutex);
                storage->first = B;
                pthread_mutex_unlock(&storage->head_mutex);
            } else {
                prev->next = B;
            }
            A->next = B->next;
            B->next = A;
            
            pthread_mutex_lock(&global_mutex);
            swap_counts[thread_id]++;
            pthread_mutex_unlock(&global_mutex);
        }
        
        pthread_mutex_unlock(&B->sync);
        pthread_mutex_unlock(&A->sync);
        if (prev) pthread_mutex_unlock(&prev->sync);
        
        usleep(1000);
    }
    return NULL;
}

void *count_monitor(void *arg) {
    int *counters = (int *)arg;
    int iterations = 0;
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    while (running && iterations < 10) {
        printf("итерация %d (MUTEX)\n", iterations + 1);
        printf("итерации: ASC=%d, DESC=%d, EQ=%d, SWAP1=%d, SWAP2=%d, SWAP3=%d\n",
               counters[ASC], counters[DESC], counters[EQ],
               counters[SWAP1], counters[SWAP2], counters[SWAP3]);
        printf("пары: asc=%d, desc=%d, eq=%d (прочитано %d, %d, %d nodes)\n",
               asc_pairs, desc_pairs, eq_pairs,
               asc_nodes_read, desc_nodes_read, eq_nodes_read);
        printf("всего свапов: %d\n",
               swap_counts[0] + swap_counts[1] + swap_counts[2]);
        sleep(2);
        iterations++;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double throughput = (counters[ASC] + counters[DESC] + counters[EQ] +
                        counters[SWAP1] + counters[SWAP2] + counters[SWAP3]) / elapsed;
    printf("затрачено времени: %.2f сек, пропуск.способность: %.0f ит/сек\n", elapsed, throughput);
    
    running = 0;
    return NULL;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    int capacity = STORAGE_CAPACITY;
    if (argc > 1) {
        capacity = atoi(argv[1]);
        if (capacity < 10) capacity = STORAGE_CAPACITY;
    }
    
    printf("емкость = %d\n", capacity);
    
    Storage *storage = initialize_storage(capacity);
    fill_storage(storage, capacity);
    
    printf("начальный список:\n");
    print_storage(storage);
    
    pthread_t threads[THREAD_COUNT + 1];
    int counters[THREAD_COUNT] = {0};
    ThreadData thread_data[THREAD_COUNT];
    
    for (int i = 0; i < THREAD_COUNT; i++) {
        thread_data[i].storage = storage;
        thread_data[i].counter = &counters[i];
        thread_data[i].thread_id = i;
    }
    
    pthread_create(&threads[ASC], NULL, ascending_thread, &thread_data[ASC]);
    pthread_create(&threads[DESC], NULL, descending_thread, &thread_data[DESC]);
    pthread_create(&threads[EQ], NULL, equal_length_thread, &thread_data[EQ]);
    pthread_create(&threads[SWAP1], NULL, swap_thread, &thread_data[SWAP1]);
    pthread_create(&threads[SWAP2], NULL, swap_thread, &thread_data[SWAP2]);
    pthread_create(&threads[SWAP3], NULL, swap_thread, &thread_data[SWAP3]);
    pthread_create(&threads[THREAD_COUNT], NULL, count_monitor, counters);
    
    pthread_join(threads[THREAD_COUNT], NULL);
    sleep(1);
    
    printf("итоговый список:\n");
    print_storage(storage);
    printf("\nИтого: ASC=%d DESC=%d EQ=%d, Swaps: S1=%d S2=%d S3=%d (всего: %d)\n",
           counters[ASC], counters[DESC], counters[EQ],
           swap_counts[0], swap_counts[1], swap_counts[2],
           swap_counts[0] + swap_counts[1] + swap_counts[2]);
    
    free_storage(storage);
    pthread_mutex_destroy(&global_mutex);
    
    printf("\n[mutex] выполнение завершено.\n");
    return 0;
}
