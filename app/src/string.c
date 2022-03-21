#include "string.h"


int string_find_substring(const string_t string, const string_t substring){
    for (int i = 0; i < string.size; i++){
        int j;
        for (j = 0; j < substring.size; j++){
            if (string.chars[i + j] != substring.chars[j]){
                break;
            }
        }

        if (j == substring.size){
            return i;
        }
    }

    return -1;
}

void string_split(const string_t string, const string_t delimiter, string_t * head, string_t * tail){
    int index = string_find_substring(string, delimiter);
    if (index < 0){
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
