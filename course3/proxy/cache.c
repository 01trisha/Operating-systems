#include "cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

cache_t *cache_create(size_t max_memory){
    cache_t *cache = calloc(1, sizeof(cache_t));
    if (!cache) return NULL;
    
    cache->max_memory = max_memory > 0 ? max_memory : DEFAULT_MAX_CACHE_SIZE;
    pthread_mutex_init(&cache->mutex, NULL);
    pthread_cond_init(&cache->gc_needed, NULL);
    
    return cache;
}

static void free_chunks(cache_chunk_t *chunk){
    while (chunk){
        cache_chunk_t *next = chunk->next;
        free(chunk);
        chunk = next;
    }
}

static void free_entry(cache_entry_t *entry){
    if (!entry) return;
    
    pthread_mutex_destroy(&entry->mutex);
    pthread_cond_destroy(&entry->data_available);
    free(entry->url);
    free(entry->headers);
    free_chunks(entry->chunks);
    free(entry);
}

static void remove_entry_from_list(cache_t *cache, cache_entry_t *entry){
    if (entry->prev){
        entry->prev->next = entry->next;
    } else{
        cache->head = entry->next;
    }
    
    if (entry->next){
        entry->next->prev = entry->prev;
    } else{
        cache->tail = entry->prev;
    }
    
    cache->entry_count--;
}

static void move_to_front(cache_t *cache, cache_entry_t *entry){
    if (entry == cache->head) return;
    
    if (entry->prev){
        entry->prev->next = entry->next;
    }
    if (entry->next){
        entry->next->prev = entry->prev;
    }
    if (entry == cache->tail){
        cache->tail = entry->prev;
    }
    
    entry->prev = NULL;
    entry->next = cache->head;
    if (cache->head){
        cache->head->prev = entry;
    }
    cache->head = entry;
    if (!cache->tail){
        cache->tail = entry;
    }
}

void cache_destroy(cache_t *cache){
    if (!cache) return;
    
    pthread_mutex_lock(&cache->mutex);
    cache->shutdown = 1;
    pthread_cond_broadcast(&cache->gc_needed);
    pthread_mutex_unlock(&cache->mutex);
    
    cache_entry_t *entry = cache->head;
    while (entry){
        cache_entry_t *next = entry->next;
        
        pthread_mutex_lock(&entry->mutex);
        pthread_cond_broadcast(&entry->data_available);
        pthread_mutex_unlock(&entry->mutex);
        
        free_entry(entry);
        entry = next;
    }
    
    pthread_mutex_destroy(&cache->mutex);
    pthread_cond_destroy(&cache->gc_needed);
    free(cache);
}

cache_entry_t *cache_get(cache_t *cache, const char *url){
    pthread_mutex_lock(&cache->mutex);
    
    cache_entry_t *entry = cache->head;
    while (entry){
        if (strcmp(entry->url, url) == 0){
            entry->ref_count++;
            entry->last_access = time(NULL);
            move_to_front(cache, entry);
            pthread_mutex_unlock(&cache->mutex);
            return entry;
        }
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&cache->mutex);
    return NULL;
}

cache_entry_t *cache_create_entry(cache_t *cache, const char *url){
    cache_entry_t *entry = calloc(1, sizeof(cache_entry_t));
    if (!entry) return NULL;
    
    entry->url = strdup(url);
    if (!entry->url){
        free(entry);
        return NULL;
    }
    
    entry->status = CACHE_ENTRY_LOADING;
    entry->ref_count = 1;
    entry->downloader_active = 1;
    entry->created = time(NULL);
    entry->last_access = entry->created;
    
    pthread_mutex_init(&entry->mutex, NULL);
    pthread_cond_init(&entry->data_available, NULL);
    
    pthread_mutex_lock(&cache->mutex);
    
    cache_entry_t *existing = cache->head;
    while (existing){
        if (strcmp(existing->url, url) == 0){
            existing->ref_count++;
            pthread_mutex_unlock(&cache->mutex);
            free_entry(entry);
            return existing;
        }
        existing = existing->next;
    }
    
    entry->next = cache->head;
    if (cache->head) cache->head->prev = entry;
    cache->head = entry;
    if (!cache->tail) cache->tail = entry;
    cache->entry_count++;
    
    cache->total_memory += sizeof(cache_entry_t) + strlen(url) + 1;
    
    if (cache->total_memory > cache->max_memory * CACHE_LOW_WATERMARK){
        pthread_cond_signal(&cache->gc_needed);
    }
    
    pthread_mutex_unlock(&cache->mutex);
    return entry;
}

void cache_entry_ref(cache_entry_t *entry){
    pthread_mutex_lock(&entry->mutex);
    entry->ref_count++;
    pthread_mutex_unlock(&entry->mutex);
}

void cache_entry_unref(cache_t *cache, cache_entry_t *entry){
    if (!entry) return;
    
    pthread_mutex_lock(&entry->mutex);
    entry->ref_count--;
    int ref_count = entry->ref_count;
    int downloader_active = entry->downloader_active;
    cache_entry_status_t status = entry->status;
    pthread_mutex_unlock(&entry->mutex);
    
    if (ref_count <= 0 && !downloader_active && status != CACHE_ENTRY_COMPLETE){
        pthread_mutex_lock(&cache->mutex);
        
        pthread_mutex_lock(&entry->mutex);
        if (entry->ref_count <= 0 && !entry->downloader_active && 
            entry->status != CACHE_ENTRY_COMPLETE){
            pthread_mutex_unlock(&entry->mutex);
            
            size_t entry_size = sizeof(cache_entry_t) + strlen(entry->url) + 1 + entry->headers_len + (entry->total_size / CACHE_CHUNK_SIZE + 1) * sizeof(cache_chunk_t);
            cache->total_memory -= entry_size;
            
            remove_entry_from_list(cache, entry);
            pthread_mutex_unlock(&cache->mutex);
            
            free_entry(entry);
            return;
        }
        pthread_mutex_unlock(&entry->mutex);
        pthread_mutex_unlock(&cache->mutex);
    }
}

int cache_entry_set_headers(cache_entry_t *entry, const char *headers, size_t len, int status_code, size_t content_length){
    pthread_mutex_lock(&entry->mutex);
    
    entry->headers = malloc(len + 1);
    if (!entry->headers){
        pthread_mutex_unlock(&entry->mutex);
        return -1;
    }
    
    memcpy(entry->headers, headers, len);
    entry->headers[len] = '\0';
    entry->headers_len = len;
    entry->http_status_code = status_code;
    entry->content_length = content_length;
    
    pthread_cond_broadcast(&entry->data_available);
    pthread_mutex_unlock(&entry->mutex);
    
    return 0;
}

int cache_entry_append_data(cache_t *cache, cache_entry_t *entry, const char *data, size_t len){
    if (len == 0) return 0;
    
    pthread_mutex_lock(&entry->mutex);
    
    size_t offset = 0;
    while (offset < len){
        cache_chunk_t *chunk = entry->last_chunk;
        
        if (!chunk || chunk->size >= CACHE_CHUNK_SIZE){
            cache_chunk_t *new_chunk = calloc(1, sizeof(cache_chunk_t));
            if (!new_chunk){
                pthread_mutex_unlock(&entry->mutex);
                return -1;
            }
            
            if (entry->last_chunk){
                entry->last_chunk->next = new_chunk;
            } else{
                entry->chunks = new_chunk;
            }
            entry->last_chunk = new_chunk;
            chunk = new_chunk;
            
            pthread_mutex_lock(&cache->mutex);
            cache->total_memory += sizeof(cache_chunk_t);
            if (cache->total_memory > cache->max_memory * CACHE_LOW_WATERMARK){
                pthread_cond_signal(&cache->gc_needed);
            }
            pthread_mutex_unlock(&cache->mutex);
        }
        
        size_t space = CACHE_CHUNK_SIZE - chunk->size;
        size_t to_copy = (len - offset < space) ? (len - offset) : space;
        
        memcpy(chunk->data + chunk->size, data + offset, to_copy);
        chunk->size += to_copy;
        entry->total_size += to_copy;
        offset += to_copy;
    }
    
    pthread_cond_broadcast(&entry->data_available);
    pthread_mutex_unlock(&entry->mutex);
    
    return 0;
}

void cache_entry_set_complete(cache_entry_t *entry){
    pthread_mutex_lock(&entry->mutex);
    entry->status = CACHE_ENTRY_COMPLETE;
    entry->downloader_active = 0;
    pthread_cond_broadcast(&entry->data_available);
    pthread_mutex_unlock(&entry->mutex);
}

void cache_entry_set_error(cache_entry_t *entry){
    pthread_mutex_lock(&entry->mutex);
    entry->status = CACHE_ENTRY_ERROR;
    entry->downloader_active = 0;
    pthread_cond_broadcast(&entry->data_available);
    pthread_mutex_unlock(&entry->mutex);
}

size_t cache_entry_read(cache_entry_t *entry, size_t offset, char *buf, size_t buf_size){
    pthread_mutex_lock(&entry->mutex);
    
    if (offset >= entry->total_size){
        pthread_mutex_unlock(&entry->mutex);
        return 0;
    }
    
    size_t chunk_idx = offset / CACHE_CHUNK_SIZE;
    size_t chunk_offset = offset % CACHE_CHUNK_SIZE;
    
    cache_chunk_t *chunk = entry->chunks;
    for (size_t i = 0; i < chunk_idx && chunk; i++){
        chunk = chunk->next;
    }
    
    size_t total_read = 0;
    while (chunk && total_read < buf_size){
        size_t available = chunk->size - chunk_offset;
        size_t to_read = (buf_size - total_read < available) ? (buf_size - total_read) : available;
        
        memcpy(buf + total_read, chunk->data + chunk_offset, to_read);
        total_read += to_read;
        chunk_offset = 0;
        chunk = chunk->next;
    }
    
    pthread_mutex_unlock(&entry->mutex);
    return total_read;
}

int cache_entry_wait_data(cache_entry_t *entry, size_t offset, int timeout_ms){
    pthread_mutex_lock(&entry->mutex);
    
    while (entry->total_size <= offset && 
           entry->status == CACHE_ENTRY_LOADING &&
           entry->downloader_active){
        
        if (timeout_ms > 0){
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += timeout_ms / 1000;
            ts.tv_nsec += (timeout_ms % 1000) * 1000000;
            if (ts.tv_nsec >= 1000000000){
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000;
            }
            
            int rc = pthread_cond_timedwait(&entry->data_available, &entry->mutex, &ts);
            if (rc == ETIMEDOUT){
                pthread_mutex_unlock(&entry->mutex);
                return -1;
            }
        } else{
            pthread_cond_wait(&entry->data_available, &entry->mutex);
        }
    }
    
    int result = 0;
    if (entry->status == CACHE_ENTRY_ERROR){
        result = -1;
    } else if (entry->total_size <= offset && 
               entry->status == CACHE_ENTRY_COMPLETE){
        result = 1;
    }
    
    pthread_mutex_unlock(&entry->mutex);
    return result;
}

void *cache_gc_thread(void *arg){
    cache_t *cache = (cache_t *)arg;
    
    pthread_mutex_lock(&cache->mutex);
    
    while (!cache->shutdown){
        while (!cache->shutdown && cache->total_memory <= cache->max_memory * CACHE_LOW_WATERMARK){
            pthread_cond_wait(&cache->gc_needed, &cache->mutex);
        }
        
        if (cache->shutdown) break;
        
        while (cache->total_memory > cache->max_memory * CACHE_LOW_WATERMARK && 
               cache->tail){
            cache_entry_t *victim = cache->tail;
            
            pthread_mutex_lock(&victim->mutex);
            if (victim->ref_count > 0 || victim->downloader_active){
                pthread_mutex_unlock(&victim->mutex);
                
                if (victim->prev){
                    victim = victim->prev;
                } else{
                    break;
                }
                continue;
            }
            pthread_mutex_unlock(&victim->mutex);
            
            size_t entry_size = sizeof(cache_entry_t) + strlen(victim->url) + 1 +
                               victim->headers_len;
            cache_chunk_t *chunk = victim->chunks;
            while (chunk){
                entry_size += sizeof(cache_chunk_t);
                chunk = chunk->next;
            }
            
            cache->total_memory -= entry_size;
            remove_entry_from_list(cache, victim);
            
            fprintf(stderr, "[GC] evicted: %s (freed %zu bytes)\n", victim->url, entry_size);
            
            pthread_mutex_unlock(&cache->mutex);
            free_entry(victim);
            pthread_mutex_lock(&cache->mutex);
        }
    }
    
    pthread_mutex_unlock(&cache->mutex);
    return NULL;
}
