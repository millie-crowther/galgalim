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
    case '/':
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

static const char * json_load_dictionary_keys(json_t * json, const char * data){
    const char * character = data + 1;
    const char * key;
    while (*character != '\0'){
        character += strcspn(character, "\"{}");
        if (*character == '{'){
            character = json_load_dictionary_keys(json, character);
        } else if (*character == '}'){
            return character + 1;
        } else if (*character == '"'){
            key = character + 1;
            character += strlen(character) + 1;
            if (*character == ':'){
                json->keys[json->key_count] = (json_key_t) {
                    .key = key,
                    .scope = data,
                };
                json->key_count++;
            }
        }
    }
    return character;
}

char * file_read(const char * filename, size_t * size){
    FILE * file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char * string = malloc(*size + 1);
    fread(string, 1, *size, file);
    string[*size] = '\0';

    fclose(file);

    return string;
}

json_type_t json_get_type(const json_t json){
    if (json.data == NULL){
        return JSON_TYPE_ERROR;
    }

    if (string_contains_character("-0123456789", *json.data)){    
        return JSON_TYPE_NUMBER;
    }

    if (memcmp(json.data, "true", 4) == 0 || memcmp(json.data, "false", 5) == 0){
        return JSON_TYPE_BOOLEAN;
    }

    if (memcmp(json.data, "null", 4) == 0){
        return JSON_TYPE_NULL;
    }

    switch (json.data[0]){
    case '{':
        return JSON_TYPE_DICTIONARY;
    case '[':
        return JSON_TYPE_LIST;
    case '"':
        return JSON_TYPE_STRING;
    default:
        return JSON_TYPE_ERROR;
    }
}

json_t json_load(const char * input_string){
    int number_of_keys = 0;
    for (const char * character = input_string; *character != '\0'; character++){
        if (*character == ':'){
            number_of_keys++;
        }
    }

    uint32_t length = strlen(input_string) + 1;
    char * data = malloc(length + number_of_keys * sizeof(json_key_t));
    json_t json = {
        .data = data,
        .keys = (json_key_t *) (data + length),
        .key_count = 0,
        .document = data,
    };

    bool is_in_string = false;
    char * output_string = data;
    for (const char * character = input_string; *character != '\0'; character++){
        char escaped_character = json_escaped_character(character);
        if (*character == '"'){
            *output_string = is_in_string ? '\0' : '"';
            is_in_string = !is_in_string;
            output_string++;
        } else if (is_in_string && escaped_character != 0){
            *output_string = escaped_character;
            output_string += 2;
        } else if (is_in_string || !isspace(*character)){
            *output_string = *character;
            output_string++;
        }
    }
    *output_string = '\0';

    if (is_in_string){
        free(data);
        return (json_t) { 0 };
    }

    json_load_dictionary_keys(&json, json.data);
    qsort(json.keys, json.key_count, sizeof(json_key_t), json_key_comparator);

    for (json_key_t * key = json.keys; key < json.keys + json.key_count - 1; key++){
        if (json_key_comparator(key, key + 1) == 0){
            free(data);
            return (json_t) { 0 };
        }
    }

    return json;
}

json_t json_dictionary_find_key(json_t json, const char * key){
    json_key_t json_key = {
        .key = key,
        .scope = json.data,
    };

    json_key_t * data_key = bsearch(&json_key, json.keys, json.key_count, sizeof(json_key_t), json_key_comparator);
    return (json_t) {
        .data = data_key == NULL ? NULL : data_key->key + strlen(key) + 2,
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

void json_free(json_t * json){
    free(json->document);
    *json = (json_t) { 0 };
}