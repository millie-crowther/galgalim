#ifndef HTTP_H
#define HTTP_H

#include "array.h"
#include "string.h"

typedef struct http_header_t {
    string_t name;
    string_t value;
} http_header_t;
typedef array_t(http_header_t) http_header_array_t;

typedef struct http_request_t {
    string_t uri;
    string_t method;
    string_t protocol;
    string_t query_parameters;
    http_header_array_t headers;
} http_request_t;

string_t http_get_header_value(http_header_array_t * headers, string_t name);
void http_build_request(http_request_t * request, const string_t buffer);
int http_start_listening(const char *port);
void http_serve_forever(const char *PORT);

#endif
