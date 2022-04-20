#include "file.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char escaped_characters[] = {
    ['"'] = '"',
    ['\\'] = '\\',
    ['b'] = '\b',
    ['f'] = '\f',
    ['n'] = '\n',
    ['r'] = '\r',
    ['t'] = '\t',
};

static int json_key_comparator(const void * a, const void * b){
    const json_key_t * ka = (const json_key_t *) a;
    const json_key_t * kb = (const json_key_t *) b;

    if (ka->scope != kb->scope){
        return (ka->scope > kb->scope) - (ka->scope < kb->scope);
    }

    return strcmp(ka->key, kb->key);
}

static char * load_dictionary_keys(json_t * json, char * scope, char * end){
    char * c = scope + 1;
    char * key;
    while (c != NULL && c < end){
        for (; c < end && !string_contains_character("\"{}", *c); c++){}
        if (c >= end){
            break;
        }
        
        if (*c == '{'){
            c = load_dictionary_keys(json, c, end);
        } else if (*c == '}'){
            return c + 1;
        } else if (*c == '"'){
            key = c + 1;
            c += strlen(c) + 1;
            if (c < end && *c == ':'){
                json->keys[json->key_count] = (json_key_t){
                    .key = key,
                    .scope = scope,
                };
                json->key_count++;
            }
        }
    }
    return NULL;
}

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

json_type_t json_get_type(const json_t json){
    if (json.data != NULL){
        switch (*json.data){
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

        if (isdigit(*json.data) || *json.data == '-'){    
            return JSON_TYPE_NUMBER;
        }
    }

    return JSON_TYPE_ERROR;
}

json_t json_load(char * input_string){
    int number_of_keys = 0;
    for (char * c = input_string; c != '\0'; c++){
        if (*c == ':'){
            number_of_keys++;
        }
    }
    
    uint32_t length = strlen(input_string);
    char * data = malloc(length + 1 + number_of_keys * sizeof(json_key_t));
    json_t json = {
        .data = data,
        .keys = data + length + 1,
        .scope = data,
        .key_count = 0,
    };

    int number_of_keys = 0;
    bool is_in_string = false;
    char * output_string = json.data;
    for (char * c = input_string; *c != '\0'; c++){
        if (c[0] == '"'){
            *output_string = is_in_string ? '\0' : '"';
            is_in_string = !is_in_string;
            output_string++;
        } else if (is_in_string && c[0] == '\\' && string_contains_character("\"\\bnfrt", c[1])){
            *output_string = escaped_characters[c[1]];
            output_string += 2;
        } else if (is_in_string || !isspace(c[0])){
            *output_string = c[0];
            output_string++;
        }
    }
    *output_string = '\0';

    load_dictionary_keys(&json, json.data, json.data + length);
    qsort(json.keys, json.key_count, sizeof(json_key_t), json_key_comparator);

    return json;
}

json_t json_dictionary_find_key(json_t json, const char * key){
    if (json.scope == NULL || json_get_type(json) != JSON_TYPE_DICTIONARY){
        return (json_t){ .data = NULL };
    }

    json_key_t json_key = {
        .key = key,
        .scope = json.scope
    };

    json_key_t * data_key = (json_key_t *) bsearch(&json_key, json.keys, json.key_count, sizeof(json_key_t), json_key_comparator);
    if (data_key == NULL){
        return (json_t){ .data = NULL };
    }

    char * data = data_key->key + strlen(key) + 2;
    return (json_t){
        .data = data,
        .scope = *data == '{' ? data : NULL,
        .keys = json.keys,
        .key_count = json.key_count,
    };
}

bool json_get_boolean(const json_t json){
    return memcmp(json.data, "true", 4) == 0;
}

int64_t json_get_integer(const json_t json){
    return strtol(json.data, NULL, 10);
}

double json_get_float(const json_t json){
    return strtod(json.data, NULL);
}

const char * json_get_string(const json_t json){
    return json_get_type(json) == JSON_TYPE_STRING ? json.data + 1 : NULL;
}
