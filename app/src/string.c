#include "string.h"

#include <stdio.h>
#include <string.h>

bool char_is_whitespace(char c){
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

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
    for (int i = 0; i < string.size; i++){
        if (string.chars[i] == delimiter){
            *head = (string_t) {
                .chars = string.chars,
                .size = i
            };

            *tail = (string_t){
                .chars = string.chars + i + 1,
                .size = string.size - i - 1
            };
            return;
        }
    }

    *head = string;
    *tail = empty_string;
}

bool string_equals(const string_t a, const string_t b){
    return a.size == b.size && strncmp(a.chars, b.chars, a.size) == 0;
}

void string_print(const string_t string){
    printf("%.*s", string.size, string.chars);
}


string_t string_strip(const string_t string){
    string_t stripped = string;
    
    while (!string_is_empty(stripped) && char_is_whitespace(stripped.chars[0])){
        stripped.chars++;
        stripped.size--;
    }

    while (!string_is_empty(stripped) && char_is_whitespace(stripped.chars[stripped.size - 1])){
        stripped.size--;
    }

    return stripped;
}