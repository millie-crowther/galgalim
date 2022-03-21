#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define array_t(T)                                                               \
    struct {                                                                     \
        size_t size;                                                             \
        size_t capacity;                                                         \
        size_t element_size;                                                     \
        T * data;                                                                \
    }
typedef array_t(uint8_t) array_base_t;

#define ARRAY_CAST(x) ((array_base_t *)x)

#define array_new(x) {.size = 0, .capacity = 0, .element_size = sizeof(x), .data = NULL}

#define array_clear(x) array_base_clear(ARRAY_CAST(x))
void array_base_clear(array_base_t *a);

#define array_is_empty(x) array_base_is_empty(ARRAY_CAST(x))
bool array_base_is_empty(array_base_t *a);

#define array_push_back(x) array_base_push_back(ARRAY_CAST(x))
void array_base_push_back(array_base_t *a);

#endif