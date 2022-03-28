#include "array.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_GROWTH 2

bool array_base_is_empty(array_base_t * array) { 
    return array->size == 0; 
}

void array_base_clear(array_base_t * array) {
    array->size = 0;
    array->capacity = 0;
    free(array->data);
    array->data = NULL;
}

void array_base_push_back(array_base_t * array) {
    if (array->size == 0){
        array->data = malloc(array->element_size);
        array->size = 1;
        array->capacity = 1;
    } else {
        array->size++;
        
        if (array->size > array->capacity){
            uint32_t new_capacity = array->capacity * ARRAY_GROWTH;
            uint8_t * new_data = calloc(new_capacity, array->element_size);
            memcpy(new_data, array->data, array->capacity * array->element_size);
            free(array->data);
            array->data = new_data;
            array->capacity = new_capacity;
        }
    }
}


void array_base_pop_back(array_base_t * array){
    if (array->size <= 1){
        array_base_clear(array);
    } else {
        array->size--;

        if (array->size < array->capacity / ARRAY_GROWTH){
            array->capacity /= ARRAY_GROWTH;
            array->data = realloc(array->data, array->capacity);
        }
    }
}

uint8_t * array_base_back(array_base_t * array){
    return array->data + (array->size - 1) * array->element_size;
}