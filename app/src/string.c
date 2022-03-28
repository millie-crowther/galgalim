#include "string.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

string_t string_literal(const char * string){
    return (string_t) {
        .chars = string,
        .size = strlen(string)
    };
}

bool string_is_empty(const string_t string){
    return string.size == 0;
}

void string_split(const string_t string, char delimiter, string_t * head, string_t * tail){
    char * character = memchr(string.chars, delimiter, string.size);
    if (character == NULL){
        *head = string;
        *tail = empty_string;
    } else {
        int i = character - string.chars;
        *head = (string_t) {
            .chars = string.chars,
            .size = i
        };
        *tail = (string_t) {
            .chars = character + 1,
            .size = string.size - i - 1
        };
    }
}

bool string_equals(const string_t a, const string_t b){
    return a.size == b.size && strncmp(a.chars, b.chars, a.size) == 0;
}

string_t string_strip(const string_t string){
    string_t stripped = string;
    
    while (!string_is_empty(stripped) && isspace(stripped.chars[0])){
        stripped.chars++;
        stripped.size--;
    }

    while (!string_is_empty(stripped) && isspace(stripped.chars[stripped.size - 1])){
        stripped.size--;
    }

    return stripped;
}


bool string_starts_with(const string_t string, const string_t prefix){
    return prefix.size <= string.size && strncmp(string.chars, prefix.chars, prefix.size) == 0;
}