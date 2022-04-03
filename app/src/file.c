#include "file.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char * file_read(const char * filename){
    FILE * file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char * string = malloc(size + 1);
    fread(string, 1, size, file);
    string[size] = '\0';

    fclose(file);

    return string;
}

json_type_t json_infer_type(const char * string){
    switch (*string){
    case '{':
        return JSON_TYPE_DICTIONARY;
    case '[':
        return JSON_TYPE_LIST;
    case '"':
        return JSON_TYPE_STRING;
    case 't':
    case 'f':
        return JSON_TYPE_BOOLEAN;
    case 'n':
        return JSON_TYPE_NULL; 
    }

    if (isdigit(*string) || *string == '-'){
        string++;
        while (isdigit(*string)){
            string++;
        }
        return *string == '.' || toupper(*string) == 'E' ? JSON_TYPE_FLOAT : JSON_TYPE_INTEGER;
    }

    return JSON_TYPE_ERROR;
}

json_t json_load(char * buffer, uint32_t length){
    bool is_in_string = false;
    char * character = buffer;
    for (uint32_t i = 0; i < length; i++){
        if (buffer[i] == '"' && (i == 0 || buffer[i - 1] != '\'')){
            is_in_string = !is_in_string;
        }

        if (is_in_string || !isspace(buffer[i])){
            *character = buffer[i];
            character++;
        }
    }

    uint32_t new_length = character - buffer;
    memset(character, 0, length - new_length);
    return (json_t) {
        .buffer = buffer,
        .length = new_length,
        .type = json_infer_type(buffer),
    };
}

char * json_skip_string(char * start, const char * end){
    start++;
    for (; start < end && *start != '"'; start += 1 + (*start == '\\')){}
    start++;
    return start;
}

char * json_skip_collection(char * start, const char * end, const char open_bracket){
    char close_bracket = open_bracket == '[' ? ']' : '}';
    start++;
    while (start < end && *start != close_bracket){
        if (*start == open_bracket){
            start = json_skip_collection(start, end, open_bracket);
        } else if (*start == '"'){
            start = json_skip_string(start, end);
        } else {
            start++;
        }
    }
    start++;
    return start;
}

json_t json_dictionary_find_key(json_t json, const string_t key){
    char key_quoted[key.size + 4];
    sprintf(key_quoted, "\"%.*s\":", key.size, key.chars);
    string_t key_quoted_string = string_new(key_quoted);

    char * start = json.buffer;
    const char * end = start + json.length;

    start++;
    while (start < end && *start != '}'){ 
        bool is_found = string_starts_with((string_t){.chars = start, .size = end - start}, key_quoted_string);
        start = json_skip_string(start, end);
        start++;

        if (start < end){
            if (is_found){
                return (json_t) {
                    .buffer = start,
                    .length = end - start,
                    .type = json_infer_type(start)
                };
                
            } else if (*start == '[' || *start == '{'){
                start = json_skip_collection(start, end, *start);
            } else if (*start == '"'){
                start = json_skip_string(start, end);
            } 
            for (; start < end && *start != ',' && *start != '}'; start++){}
            start += start < end && *start == ',';
        }
    }

    return (json_t) {
        .type = JSON_TYPE_ERROR
    };
}
