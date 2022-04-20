#include "string.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

const char * string_split(const char * string, const char * delimiter){
    char * split = strstr(string, delimiter);
    if (split == NULL){
        return NULL;
    }
    split[0] = '\0';
    return split + strlen(delimiter);
}

bool string_starts_with(const char * string, const char * prefix){
    return memcmp(string, prefix, strlen(prefix)) == 0;
}

bool string_contains_character(const char * string, const char character){
    return strchr(string, character) != NULL;
}

bool string_equals(const char * a, const char * b){
    return a == b || strcmp(a, b) == 0;
}