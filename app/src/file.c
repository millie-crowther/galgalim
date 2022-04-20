#include "file.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char json_escaped_character(const char * string){
    if (string[0] != '\\'){
        return 0;
    }

    switch (string[1]){
    case '\\':
    case '"':
        return string[1];
    case 'b':
        return '\b';
    case 'n':
        return '\n';
    case 'f':
        return '\f';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    }
    
    return 0;
}

static int json_key_comparator(const void * a, const void * b){
    const json_key_t * ka = (const json_key_t *) a;
    const json_key_t * kb = (const json_key_t *) b;

    if (ka->scope != kb->scope){
        return (ka->scope > kb->scope) - (ka->scope < kb->scope);
    }

    return strcmp(ka->key, kb->key);
}

static const char * json_load_dictionary_keys(json_t * json, const char * data, const char * end){
    const char * c = data + 1;
    const char * key;
    while (c != NULL && c < end){
        for (; c[0] != '\0' && !string_contains_character("\"{}", *c); c++){}
        if (*c == '{'){
            c = json_load_dictionary_keys(json, c, end);
        } else if (*c == '}'){
            return c + 1;
        } else if (*c == '"'){
            key = c + 1;
            c += strlen(c) + 1;
            if (c < end && c[0] == ':'){
                json->keys[json->key_count] = (json_key_t){
                    .key = key,
                    .scope = data,
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
    for (char * c = input_string; *c != '\0'; c++){
        if (*c == ':'){
            number_of_keys++;
        }
    }
    
    uint32_t length = strlen(input_string);
    char * data = malloc(length + 1 + number_of_keys * sizeof(json_key_t));
    json_t json = {
        .data = data,
        .keys = (json_key_t *) data + length + 1,
        .key_count = 0,
    };

    bool is_in_string = false;
    char * output_string = data;
    for (char * c = input_string; *c != '\0'; c++){
        char escaped_character = json_escaped_character(c);
        if (c[0] == '"'){
            *output_string = is_in_string ? '\0' : '"';
            is_in_string = !is_in_string;
            output_string++;
        } else if (is_in_string && escaped_character != 0){
            *output_string = escaped_character;
            output_string += 2;
        } else if (is_in_string || !isspace(c[0])){
            *output_string = c[0];
            output_string++;
        }
    }
    *output_string = '\0';

    json_load_dictionary_keys(&json, json.data, json.data + length);
    qsort(json.keys, json.key_count, sizeof(json_key_t), json_key_comparator);

    return json;
}

json_t json_dictionary_find_key(json_t json, const char * key){
    if (json_get_type(json) != JSON_TYPE_DICTIONARY){
        return (json_t){ .data = NULL };
    }

    json_key_t json_key = {
        .key = key,
        .scope = json.data
    };

    json_key_t * data_key = (json_key_t *) bsearch(&json_key, json.keys, json.key_count, sizeof(json_key_t), json_key_comparator);
    if (data_key == NULL){
        return (json_t){ .data = NULL };
    }

    const char * data = data_key->key + strlen(key) + 2;
    return (json_t){
        .data = data,
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
