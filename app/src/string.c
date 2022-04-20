#include "string.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

string_t string_new(const char * string){
    return (string_t) {
        .chars = string,
        .size = strlen(string)
    };
}

bool string_is_empty(const string_t string){
    return string.size == 0;
}

void string_split(const string_t string, const char * delimiter, string_t * head, string_t * tail){
    *tail = string;
    int delimiter_size = strlen(delimiter);
    while (!string_starts_with(tail->chars, delimiter) && tail->size >= delimiter_size){
        tail->chars++;
        tail->size--;
    }

    if (tail->size < delimiter_size){
        *head = string;
        *tail = empty_string;
    } else {
        head->chars = string.chars;
        head->size = string.size - tail->size;
        tail->chars += delimiter_size;
        tail->size -= delimiter_size;
    }
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