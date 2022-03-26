#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include <stdint.h>

typedef struct string_t {
    const char * chars;
    uint32_t size;
} string_t;

#define empty_string (string_t){.chars = "", .size = 0}

bool char_is_whitespace(char c);

bool string_is_empty(const string_t string);
bool string_equals(const string_t a, const string_t b);
string_t string_literal(const char * string);
void string_split(const string_t string, char delimiter, string_t * head, string_t * tail);
string_t string_strip(const string_t string);
bool string_contains_character(const string_t string, const char character);

#endif