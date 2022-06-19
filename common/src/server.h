#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>

typedef struct http_request_t {
    const char * uri;
    const char * method;
    const char * protocol;
    const char * query_parameters;
    const char * headers;
    const char * payload;
} http_request_t;

typedef void (*http_route_handler_t)(const http_request_t * request, void * db, FILE * output);  

void http_build_request(http_request_t * request, char * buffer);
int http_start_listening(const char * port);
void http_serve_forever(const char * port, void * db, http_route_handler_t route_handler);

#endif
