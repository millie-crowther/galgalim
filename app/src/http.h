#ifndef HTTP_H
#define HTTP_H

#include "array.h"
#include "string.h"

typedef struct http_request_t {
    string_t request_line_string;
    string_t uri;
    string_t method;
    string_t protocol;
    string_t query_parameters;
    string_t headers;
    char * payload;
    uint32_t payload_length;
} http_request_t;

void http_build_request(http_request_t * request, char * buffer, uint32_t length);
int http_start_listening(const char *port);
void http_serve_forever(const char *PORT);
void http_close_socket(const int file_descriptor);
string_t http_header_value(const http_request_t * request, const string_t header_name);

#endif
