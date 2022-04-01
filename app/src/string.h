#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include <stdint.h>

typedef struct string_t {
    const char * chars;
    uint32_t size;
} string_t;

#define empty_string (string_t){.chars = "", .size = 0}

bool string_is_empty(const string_t string);
bool string_equals(const string_t a, const string_t b);
string_t string_new(const char * string);
void string_split(const string_t string, char delimiter, string_t * head, string_t * tail);
string_t string_strip(const string_t string);
bool string_starts_with(const string_t string, const string_t prefix);

#endif