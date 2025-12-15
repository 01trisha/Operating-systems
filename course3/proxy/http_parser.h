#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stddef.h>

#define MAX_HEADERS 100
#define MAX_URL_LEN 4096
#define MAX_HEADER_LEN 8192

typedef struct{
    char method[16];
    char url[MAX_URL_LEN];
    char host[256];
    int port;
    char path[MAX_URL_LEN];
    int minor_version;
    
    struct{
        char name[256];
        char value[1024];
    } headers[MAX_HEADERS];
    int num_headers;
    
    size_t headers_len;
} http_request_t;

typedef struct{
    int status_code;
    int minor_version;
    char status_msg[256];
    
    struct{
        char name[256];
        char value[1024];
    } headers[MAX_HEADERS];
    int num_headers;
    
    size_t content_length;
    int chunked;
    int keep_alive;
    
    size_t headers_len;
} http_response_t;

int http_parse_request(const char *buf, size_t len, http_request_t *req);

int http_parse_response(const char *buf, size_t len, http_response_t *resp);

int http_build_request(const http_request_t *req, char *buf, size_t buf_size);

const char *http_get_header(const http_request_t *req, const char *name);
const char *http_response_get_header(const http_response_t *resp, const char *name);

void http_build_cache_key(const http_request_t *req, char *key, size_t key_size);

#endif
