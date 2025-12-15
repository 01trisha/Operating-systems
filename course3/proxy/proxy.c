#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <getopt.h>

#include "cache.h"
#include "http_parser.h"

#define DEFAULT_PORT 8080
#define RECV_BUF_SIZE 16384
#define MAX_REQUEST_SIZE 65536

static cache_t *g_cache = NULL;
static int g_server_fd = -1;
static volatile sig_atomic_t g_shutdown = 0;
static pthread_t g_gc_thread;
static pthread_mutex_t g_threads_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_active_threads = 0;

typedef struct{
    int client_fd;
    struct sockaddr_in client_addr;
} client_ctx_t;

static void *client_thread(void *arg);
static int connect_to_server(const char *host, int port);
static int download_and_cache(cache_t *cache, cache_entry_t *entry, 
                              const http_request_t *req);
static int send_cached_response(int client_fd, cache_entry_t *entry);
static int send_error_response(int client_fd, int status, const char *msg);

static void signal_handler(int sig){
    (void)sig;
    g_shutdown = 1;
    if (g_server_fd >= 0){
        shutdown(g_server_fd, SHUT_RDWR);
    }
}

static void setup_signals(void){
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    signal(SIGPIPE, SIG_IGN);
}

static int create_server_socket(int port){
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0){
        perror("socket");
        return -1;
    }
    
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("bind");
        close(fd);
        return -1;
    }
    
    if (listen(fd, SOMAXCONN) < 0){
        perror("listen");
        close(fd);
        return -1;
    }
    
    return fd;
}

static void print_usage(const char *prog){
    fprintf(stderr, "Usage: %s [OPTIONS]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -p, --port PORT      Listen port (default: %d)\n", DEFAULT_PORT);
    fprintf(stderr, "  -c, --cache-size MB  Max cache size in MB (default: 100)\n");
    fprintf(stderr, "  -h, --help           Show this help\n");
}

int main(int argc, char *argv[]){
    int port = DEFAULT_PORT;
    size_t cache_size = DEFAULT_MAX_CACHE_SIZE;
    
    static struct option long_options[] ={
       {"port", required_argument, 0, 'p'},
       {"cache-size", required_argument, 0, 'c'},
       {"help", no_argument, 0, 'h'},
       {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "p:c:h", long_options, NULL)) != -1){
        switch (opt){
            case 'p':
                port = atoi(optarg);
                break;
            case 'c':
                cache_size = (size_t)atoi(optarg) * 1024 * 1024;
                break;
            case 'h':
            default:
                print_usage(argv[0]);
                return opt == 'h' ? 0 : 1;
        }
    }
    
    setup_signals();
    
    g_cache = cache_create(cache_size);
    if (!g_cache){
        fprintf(stderr, "Failed to create cache\n");
        return 1;
    }
    
    if (pthread_create(&g_gc_thread, NULL, cache_gc_thread, g_cache) != 0){
        fprintf(stderr, "Failed to create GC thread\n");
        cache_destroy(g_cache);
        return 1;
    }
    
    g_server_fd = create_server_socket(port);
    if (g_server_fd < 0){
        cache_destroy(g_cache);
        return 1;
    }
    
    fprintf(stderr, "HTTP proxy listening on port %d (cache: %zu MB)\n", 
            port, cache_size / (1024 * 1024));
    
    while (!g_shutdown){
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_fd = accept(g_server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0){
            if (errno == EINTR || g_shutdown) break;
            perror("accept");
            continue;
        }
        
        client_ctx_t *ctx = malloc(sizeof(client_ctx_t));
        if (!ctx){
            close(client_fd);
            continue;
        }
        
        ctx->client_fd = client_fd;
        ctx->client_addr = client_addr;
        
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        
        if (pthread_create(&tid, &attr, client_thread, ctx) != 0){
            perror("pthread_create");
            close(client_fd);
            free(ctx);
        } else{
            pthread_mutex_lock(&g_threads_mutex);
            g_active_threads++;
            pthread_mutex_unlock(&g_threads_mutex);
        }
        
        pthread_attr_destroy(&attr);
    }
    
    fprintf(stderr, "\nShutting down...\n");
    
    if (g_server_fd >= 0){
        close(g_server_fd);
        g_server_fd = -1;
    }
    
    int wait_count = 0;
    while (1){
        pthread_mutex_lock(&g_threads_mutex);
        int active = g_active_threads;
        pthread_mutex_unlock(&g_threads_mutex);
        
        if (active == 0) break;
        if (wait_count++ > 50){
            fprintf(stderr, "Timeout waiting for %d threads\n", active);
            break;
        }
        
        struct timespec ts ={0, 100000000};
        nanosleep(&ts, NULL);
    }
    
    cache_destroy(g_cache);
    pthread_join(g_gc_thread, NULL);
    
    fprintf(stderr, "Proxy stopped\n");
    return 0;
}

static void *client_thread(void *arg){
    client_ctx_t *ctx = (client_ctx_t *)arg;
    int client_fd = ctx->client_fd;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ctx->client_addr.sin_addr, client_ip, sizeof(client_ip));
    free(ctx);
    
    char buf[MAX_REQUEST_SIZE];
    size_t buf_len = 0;
    http_request_t req;
    
    while (buf_len < sizeof(buf) - 1){
        struct pollfd pfd ={client_fd, POLLIN, 0};
        int ret = poll(&pfd, 1, 30000);
        
        if (ret <= 0 || g_shutdown){
            goto cleanup;
        }
        
        ssize_t n = recv(client_fd, buf + buf_len, sizeof(buf) - buf_len - 1, 0);
        if (n <= 0){
            goto cleanup;
        }
        buf_len += n;
        buf[buf_len] = '\0';
        
        int parsed = http_parse_request(buf, buf_len, &req);
        if (parsed > 0){
            break;
        } else if (parsed == -1){
            send_error_response(client_fd, 400, "Bad Request");
            goto cleanup;
        }
    }
    
    if (strcmp(req.method, "GET") != 0){
        send_error_response(client_fd, 501, "Not Implemented");
        goto cleanup;
    }
    
    if (req.host[0] == '\0'){
        send_error_response(client_fd, 400, "Bad Request - No Host");
        goto cleanup;
    }
    
    char cache_key[MAX_URL_LEN + 64];
    http_build_cache_key(&req, cache_key, sizeof(cache_key));
    
    fprintf(stderr, "[%s] GET %s\n", client_ip, cache_key);
    
    cache_entry_t *entry = cache_get(g_cache, cache_key);
    
    if (entry){
        fprintf(stderr, "[%s] Cache HIT: %s\n", client_ip, cache_key);
        send_cached_response(client_fd, entry);
        cache_entry_unref(g_cache, entry);
    } else{
        entry = cache_create_entry(g_cache, cache_key);
        if (!entry){
            send_error_response(client_fd, 503, "Service Unavailable");
            goto cleanup;
        }
        
        pthread_mutex_lock(&entry->mutex);
        int is_downloader = entry->downloader_active || entry->headers == NULL;
        pthread_mutex_unlock(&entry->mutex);
        
        if (is_downloader){
            fprintf(stderr, "[%s] Cache MISS, downloading: %s\n", client_ip, cache_key);
            
            if (download_and_cache(g_cache, entry, &req) < 0){
                cache_entry_set_error(entry);
                send_error_response(client_fd, 502, "Bad Gateway");
            } else{
                send_cached_response(client_fd, entry);
            }
        } else{
            fprintf(stderr, "[%s] Cache LOADING, waiting: %s\n", client_ip, cache_key);
            send_cached_response(client_fd, entry);
        }
        
        cache_entry_unref(g_cache, entry);
    }
    
cleanup:
    close(client_fd);
    
    pthread_mutex_lock(&g_threads_mutex);
    g_active_threads--;
    pthread_mutex_unlock(&g_threads_mutex);
    
    return NULL;
}

static int connect_to_server(const char *host, int port){
    struct addrinfo hints, *res, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    int err = getaddrinfo(host, port_str, &hints, &res);
    if (err != 0){
        fprintf(stderr, "getaddrinfo(%s): %s\n", host, gai_strerror(err));
        return -1;
    }
    
    int fd = -1;
    for (rp = res; rp != NULL; rp = rp->ai_next){
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) continue;
        
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0){
            break;
        }
        
        close(fd);
        fd = -1;
    }
    
    freeaddrinfo(res);
    return fd;
}

static int download_and_cache(cache_t *cache, cache_entry_t *entry, const http_request_t *req){
    int server_fd = connect_to_server(req->host, req->port);
    if (server_fd < 0){
        return -1;
    }
    
    char request_buf[MAX_REQUEST_SIZE];
    int req_len = http_build_request(req, request_buf, sizeof(request_buf));
    if (req_len < 0){
        close(server_fd);
        return -1;
    }
    
    if (send(server_fd, request_buf, req_len, 0) != req_len){
        close(server_fd);
        return -1;
    }
    
    char recv_buf[RECV_BUF_SIZE];
    size_t total_recv = 0;
    int headers_parsed = 0;
    http_response_t resp;
    size_t body_received = 0;
    
    while (!g_shutdown){
        struct pollfd pfd ={server_fd, POLLIN, 0};
        int ret = poll(&pfd, 1, 60000);
        
        if (ret <= 0){
            close(server_fd);
            return -1;
        }
        
        ssize_t n = recv(server_fd, recv_buf, sizeof(recv_buf), 0);
        if (n < 0){
            close(server_fd);
            return -1;
        }
        if (n == 0) break;
        
        size_t offset = 0;
        
        if (!headers_parsed){
            static __thread char header_buf[MAX_HEADER_LEN];
            static __thread size_t header_buf_len = 0;
            
            size_t copy_len = (size_t)n;
            if (header_buf_len + copy_len > sizeof(header_buf)){
                copy_len = sizeof(header_buf) - header_buf_len;
            }
            memcpy(header_buf + header_buf_len, recv_buf, copy_len);
            header_buf_len += copy_len;
            
            int parsed = http_parse_response(header_buf, header_buf_len, &resp);
            if (parsed == -1){
                close(server_fd);
                header_buf_len = 0;
                return -1;
            }
            if (parsed > 0){
                headers_parsed = 1;
                offset = parsed - (header_buf_len - n);
                
                cache_entry_set_headers(entry, header_buf, parsed, resp.status_code, resp.content_length);
                
                if (resp.status_code != 200){
                    fprintf(stderr, "Not caching: status %d\n", resp.status_code);
                }
                
                header_buf_len = 0;
            }
        }
        
        if (headers_parsed && offset < (size_t)n){
            cache_entry_append_data(cache, entry, recv_buf + offset, n - offset);
            body_received += n - offset;
        }
        
        total_recv += n;
        
        if (headers_parsed && resp.content_length > 0 && body_received >= resp.content_length){
            break;
        }
    }
    
    close(server_fd);
    cache_entry_set_complete(entry);
    
    fprintf(stderr, "Downloaded %zu bytes (body: %zu)\n", total_recv, body_received);
    return 0;
}

static int send_cached_response(int client_fd, cache_entry_t *entry){
    int wait_result = cache_entry_wait_data(entry, 0, 0);
    if (wait_result < 0){
        return -1;
    }
    
    pthread_mutex_lock(&entry->mutex);
    if (entry->headers && entry->headers_len > 0){
        ssize_t sent = send(client_fd, entry->headers, entry->headers_len, 0);
        if (sent < 0){
            pthread_mutex_unlock(&entry->mutex);
            return -1;
        }
    }
    pthread_mutex_unlock(&entry->mutex);
    
    size_t offset = 0;
    char buf[RECV_BUF_SIZE];
    
    while (!g_shutdown){
        size_t read = cache_entry_read(entry, offset, buf, sizeof(buf));
        
        if (read > 0){
            ssize_t sent = send(client_fd, buf, read, 0);
            if (sent < 0){
                return -1;
            }
            offset += read;
        } else{
            int result = cache_entry_wait_data(entry, offset, 5000);
            if (result == 1){
                break; 
            } else if (result < 0){
                return -1;
            }
        }
    }
    
    return 0;
}

static int send_error_response(int client_fd, int status, const char *msg){
    char buf[512];
    int len = snprintf(buf, sizeof(buf),
        "HTTP/1.0 %d %s\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%d %s\n",
        status, msg, status, msg);
    
    send(client_fd, buf, len, 0);
    return 0;
}
