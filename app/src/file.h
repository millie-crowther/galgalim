#ifndef FILE_H
#define FILE_H

#include <stdint.h>

#include "string.h"

typedef enum json_type_t {
    JSON_TYPE_NULL,
    JSON_TYPE_BOOLEAN,
    JSON_TYPE_STRING,
    JSON_TYPE_NUMBER,
    JSON_TYPE_DICTIONARY,
    JSON_TYPE_LIST,
    JSON_TYPE_ERROR
} json_type_t;

typedef struct json_t {
    json_type_t type;
    char * buffer;
    uint32_t length;
} json_t;

char * file_read(const char * filename);

json_t json_load(char * buffer, uint32_t length);
json_t json_dictionary_find_key(json_t json, const string_t key);
bool json_get_boolean(const json_t json);
int64_t json_get_integer(const json_t json);
double json_get_float(const json_t json);
string_t json_get_string(const json_t json);

#endif