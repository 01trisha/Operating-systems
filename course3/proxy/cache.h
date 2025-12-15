#ifndef CACHE_H
#define CACHE_H

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#define CACHE_CHUNK_SIZE 8192
#define DEFAULT_MAX_CACHE_SIZE (100 * 1024 * 1024)
#define CACHE_LOW_WATERMARK 0.7 

typedef enum{
    CACHE_ENTRY_LOADING,
    CACHE_ENTRY_COMPLETE,
    CACHE_ENTRY_ERROR
} cache_entry_status_t;

typedef struct cache_chunk{
    char data[CACHE_CHUNK_SIZE];
    size_t size;
    struct cache_chunk *next;
} cache_chunk_t;

typedef struct cache_entry{
    char *url; //key
    char *headers; 
    size_t headers_len;
    
    cache_chunk_t *chunks;
    cache_chunk_t *last_chunk;
    size_t total_size;
    size_t content_length;
    
    cache_entry_status_t status;
    int http_status_code;
    
    int ref_count;
    int downloader_active;
    
    time_t last_access; //lru
    time_t created;
    
    pthread_mutex_t mutex;
    pthread_cond_t data_available;//сигналит когда данные приходят новые
    
    struct cache_entry *prev;
    struct cache_entry *next;
} cache_entry_t;

typedef struct{
    cache_entry_t *head;
    cache_entry_t *tail;
    size_t total_memory;
    size_t max_memory;
    size_t entry_count;
    
    pthread_mutex_t mutex;
    pthread_cond_t gc_needed;
    
    int shutdown;
} cache_t;

cache_t *cache_create(size_t max_memory);
void cache_destroy(cache_t *cache);

cache_entry_t *cache_get(cache_t *cache, const char *url);
cache_entry_t *cache_create_entry(cache_t *cache, const char *url);
void cache_entry_ref(cache_entry_t *entry);
void cache_entry_unref(cache_t *cache, cache_entry_t *entry);

int cache_entry_set_headers(cache_entry_t *entry, const char *headers, size_t len, int status_code, size_t content_length);
int cache_entry_append_data(cache_t *cache, cache_entry_t *entry, const char *data, size_t len);
void cache_entry_set_complete(cache_entry_t *entry);
void cache_entry_set_error(cache_entry_t *entry);

size_t cache_entry_read(cache_entry_t *entry, size_t offset, char *buf, size_t buf_size);
int cache_entry_wait_data(cache_entry_t *entry, size_t offset, int timeout_ms);

void *cache_gc_thread(void *arg);

#endif
