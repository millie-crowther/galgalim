#ifndef STRING_H
#define STRING_H

#include <stdbool.h>
#include <stdint.h>


const char * string_split(const char * string, const char * delimiter);
bool string_equals(const char * a, const char * b);
bool string_starts_with(const char * string, const char * prefix);
bool string_contains_character(const char * string, const char character);

#endif