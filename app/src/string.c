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

void string_split(const string_t string, const string_t delimiter, string_t * head, string_t * tail){
    *tail = string;
    while (!string_starts_with(*tail, delimiter) && tail->size >= delimiter.size){
        tail->chars++;
        tail->size--;
    }

    if (tail->size < delimiter.size){
        *head = string;
        *tail = empty_string;
    } else {
        head->chars = string.chars;
        head->size = string.size - tail->size;
        tail->chars += delimiter.size;
        tail->size -= delimiter.size;
    }
}

bool string_equals(const string_t a, const string_t b){
    return a.size == b.size && strncmp(a.chars, b.chars, a.size) == 0;
}

bool string_starts_with(const string_t string, const string_t prefix){
    return prefix.size <= string.size && strncmp(string.chars, prefix.chars, prefix.size) == 0;
}
