#include "string.h"

#include <string.h>

string_t string_literal(const char * string){
    return (string_t) {
        .chars = string,
        .size = strlen(string)
    };
}

uint32_t string_find_substring(const string_t string, const string_t substring){
    for (uint32_t i = 0; i < string.size; i++){
        uint32_t j;
        for (j = 0; j < substring.size; j++){
            if (string.chars[i + j] != substring.chars[j]){
                break;
            }
        }

        if (j == substring.size){
            return i;
        }
    }

    return string.size;
}

void string_split(const string_t string, const string_t delimiter, string_t * head, string_t * tail){
    uint32_t index = string_find_substring(string, delimiter);
    if (index == string.size){
        *head = string;
        *tail = empty_string;
        return;
    }

    *head = (string_t) {
        .chars = string.chars,
        .size = index
    };

    *tail = (string_t) {
        .chars = string.chars + index + delimiter.size,
        .size = string.size - index - delimiter.size
    };
}

bool string_equals(const string_t a, const string_t b){
    return a.size == b.size && strncmp(a.chars, b.chars, a.size) == 0;
}
