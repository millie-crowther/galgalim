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

typedef struct json_key_t {
    const char * key;
    const char * scope;
} json_key_t;

typedef struct json_t {
    const char * data;
    json_key_t * keys;
    uint32_t key_count;
} json_t;

char * file_read(const char * filename);

json_t json_load(const char * data);
json_t json_dictionary_find_key(json_t json, const char * key);
bool json_get_boolean(const json_t json);
int64_t json_get_integer(const json_t json);
double json_get_float(const json_t json);
const char * json_get_string(const json_t json);
json_type_t json_get_type(const json_t json);
void json_free(json_t * json);

#endif