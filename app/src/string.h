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
string_t string_new(const char * string);
void string_split(const string_t string, const char * delimiter, string_t * head, string_t * tail);

bool string_equals(const char * a, const char * b);
bool string_starts_with(const char * string, const char * prefix);
bool string_contains_character(const char * string, const char character);

#endif