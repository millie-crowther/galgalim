#include "array.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_GROWTH 2

bool array_base_is_empty(array_base_t *a) { 
    return a->size == 0; 
}

void array_base_clear(array_base_t *a) {
    a->size = 0;
    a->capacity = 0;
    free(a->data);
    a->data = NULL;
}

void array_base_push_back(array_base_t *a) {
    if (a->size == 0){
        free(a->data);
        a->data = malloc(a->element_size);
        a->size = 1;
        a->capacity = 1;
    } else {
        a->size++;
        
        if (a->size > a->capacity){
            uint32_t new_capacity = a->capacity * ARRAY_GROWTH;
            uint8_t * new_data = calloc(new_capacity, a->element_size);
            memcpy(new_data, a->data, a->capacity * a->element_size);
            free(a->data);
            a->data = new_data;
            a->capacity = new_capacity;
        }
    }
}
