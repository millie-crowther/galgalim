#ifndef HTTP_H
#define HTTP_H

#include <hiredis.h>

#include "array.h"
#include "string.h"

typedef struct http_request_t {
    const char * uri;
    const char * method;
    const char * protocol;
    const char * query_parameters;
    const char * headers;
    const char * payload;
} http_request_t;

void http_build_request(http_request_t * request, char * buffer);
int http_start_listening(const char * port);
void http_serve_forever(const char * port, redisContext * redis_context);
void http_close_socket(const int file_descriptor);

#endif
