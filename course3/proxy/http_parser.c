#include "http_parser.h"
#include "picohttpparser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

int http_parse_request(const char *buf, size_t len, http_request_t *req){
    const char *method, *path;
    size_t method_len, path_len;
    int minor_version;
    struct phr_header headers[MAX_HEADERS];
    size_t num_headers = MAX_HEADERS;

    int pret = phr_parse_request(buf, len, &method, &method_len, &path, &path_len, &minor_version, headers, &num_headers, 0);

    if (pret == -1) return -1;
    if (pret == -2) return -2;

    memset(req, 0, sizeof(*req));

    size_t copy_len = method_len < sizeof(req->method) - 1 ? method_len : sizeof(req->method) - 1;
    memcpy(req->method, method, copy_len);
    req->method[copy_len] = '\0';

    copy_len = path_len < sizeof(req->url) - 1 ? path_len : sizeof(req->url) - 1;
    memcpy(req->url, path, copy_len);
    req->url[copy_len] = '\0';

    req->minor_version = minor_version;
    req->headers_len = pret;

    if (strncmp(req->url, "http://", 7) == 0){
        const char *host_start = req->url + 7;
        const char *host_end = strchr(host_start, '/');
        const char *port_start = strchr(host_start, ':');

        if (port_start && (!host_end || port_start < host_end)){
            size_t host_len = port_start - host_start;
            if (host_len >= sizeof(req->host)) host_len = sizeof(req->host) - 1;
            memcpy(req->host, host_start, host_len);
            req->host[host_len] = '\0';
            req->port = atoi(port_start + 1);
        } else{
            size_t host_len = host_end ? (size_t)(host_end - host_start) : strlen(host_start);
            if (host_len >= sizeof(req->host)) host_len = sizeof(req->host) - 1;
            memcpy(req->host, host_start, host_len);
            req->host[host_len] = '\0';
            req->port = 80;
        }

        if (host_end){
            strncpy(req->path, host_end, sizeof(req->path) - 1);
        } else{
            strcpy(req->path, "/");
        }
    } else{
        size_t path_len = strlen(req->url);
        if (path_len >= sizeof(req->path)) path_len = sizeof(req->path) - 1;
        memcpy(req->path, req->url, path_len);
        req->path[path_len] = '\0';
        req->port = 80;
    }

    req->num_headers = num_headers < MAX_HEADERS ? num_headers : MAX_HEADERS;
    for (size_t i = 0; i < (size_t)req->num_headers; i++){
        copy_len = headers[i].name_len < sizeof(req->headers[i].name) - 1 ? headers[i].name_len : sizeof(req->headers[i].name) - 1;
        memcpy(req->headers[i].name, headers[i].name, copy_len);
        req->headers[i].name[copy_len] = '\0';

        copy_len = headers[i].value_len < sizeof(req->headers[i].value) - 1 ? headers[i].value_len : sizeof(req->headers[i].value) - 1;
        memcpy(req->headers[i].value, headers[i].value, copy_len);
        req->headers[i].value[copy_len] = '\0';

        if (strcasecmp(req->headers[i].name, "Host") == 0 && req->host[0] == '\0'){
            const char *port_sep = strchr(req->headers[i].value, ':');
            if (port_sep){
                size_t host_len = port_sep - req->headers[i].value;
                if (host_len >= sizeof(req->host)) host_len = sizeof(req->host) - 1;
                memcpy(req->host, req->headers[i].value, host_len);
                req->host[host_len] = '\0';
                req->port = atoi(port_sep + 1);
            } else{
                strncpy(req->host, req->headers[i].value, sizeof(req->host) - 1);
                req->port = 80;
            }
        }
    }

    return pret;
}

int http_parse_response(const char *buf, size_t len, http_response_t *resp){
    int minor_version, status;
    const char *msg;
    size_t msg_len;
    struct phr_header headers[MAX_HEADERS];
    size_t num_headers = MAX_HEADERS;

    int pret = phr_parse_response(buf, len, &minor_version, &status, &msg, &msg_len, headers, &num_headers, 0);

    if (pret == -1) return -1;
    if (pret == -2) return -2;

    memset(resp, 0, sizeof(*resp));

    resp->minor_version = minor_version;
    resp->status_code = status;
    resp->headers_len = pret;

    size_t copy_len = msg_len < sizeof(resp->status_msg) - 1 ? msg_len : sizeof(resp->status_msg) - 1;
    memcpy(resp->status_msg, msg, copy_len);
    resp->status_msg[copy_len] = '\0';

    resp->num_headers = num_headers < MAX_HEADERS ? num_headers : MAX_HEADERS;
    for (size_t i = 0; i < (size_t)resp->num_headers; i++){
        copy_len = headers[i].name_len < sizeof(resp->headers[i].name) - 1 ? headers[i].name_len : sizeof(resp->headers[i].name) - 1;
        memcpy(resp->headers[i].name, headers[i].name, copy_len);
        resp->headers[i].name[copy_len] = '\0';

        copy_len = headers[i].value_len < sizeof(resp->headers[i].value) - 1 ? headers[i].value_len : sizeof(resp->headers[i].value) - 1;
        memcpy(resp->headers[i].value, headers[i].value, copy_len);
        resp->headers[i].value[copy_len] = '\0';

        if (strcasecmp(resp->headers[i].name, "Content-Length") == 0){
            resp->content_length = strtoull(resp->headers[i].value, NULL, 10);
        }
        else if (strcasecmp(resp->headers[i].name, "Transfer-Encoding") == 0){
            if (strcasestr(resp->headers[i].value, "chunked")){
                resp->chunked = 1;
            }
        }
        else if (strcasecmp(resp->headers[i].name, "Connection") == 0){
            if (strcasecmp(resp->headers[i].value, "keep-alive") == 0){
                resp->keep_alive = 1;
            }
        }
    }

    return pret;
}

int http_build_request(const http_request_t *req, char *buf, size_t buf_size){
    int len = snprintf(buf, buf_size,
        "%s %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n",
        req->method, req->path, req->host);

    if (len < 0 || (size_t)len >= buf_size) return -1;

    for (int i = 0; i < req->num_headers; i++){
        if (strcasecmp(req->headers[i].name, "Host") == 0) continue;
        if (strcasecmp(req->headers[i].name, "Connection") == 0) continue;
        if (strcasecmp(req->headers[i].name, "Proxy-Connection") == 0) continue;

        int added = snprintf(buf + len, buf_size - len, "%s: %s\r\n", req->headers[i].name, req->headers[i].value);
        if (added < 0 || (size_t)(len + added) >= buf_size) return -1;
        len += added;
    }

    if ((size_t)(len + 2) >= buf_size) return -1;
    buf[len++] = '\r';
    buf[len++] = '\n';
    buf[len] = '\0';

    return len;
}

const char *http_get_header(const http_request_t *req, const char *name){
    for (int i = 0; i < req->num_headers; i++){
        if (strcasecmp(req->headers[i].name, name) == 0){
            return req->headers[i].value;
        }
    }
    return NULL;
}

const char *http_response_get_header(const http_response_t *resp, const char *name){
    for (int i = 0; i < resp->num_headers; i++){
        if (strcasecmp(resp->headers[i].name, name) == 0){
            return resp->headers[i].value;
        }
    }
    return NULL;
}

void http_build_cache_key(const http_request_t *req, char *key, size_t key_size){
    snprintf(key, key_size, "%s:%s:%d%s", req->method, req->host, req->port, req->path);
}
