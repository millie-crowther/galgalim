#ifndef STRING_H
#define STRING_H

#include <stdint.h>

typedef struct string_t {
    const char * chars;
    uint32_t size;
} string_t;

#define empty_string (string_t){.chars = "", .size = 0}

int string_find_substring(const string_t string, const string_t substring);
void string_split(const string_t string, const string_t delimiter, string_t * head, string_t * tail);

#endif