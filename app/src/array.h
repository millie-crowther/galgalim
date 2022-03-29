#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define array_t(T)                                                              \
    struct {                                                                    \
        size_t size;                                                            \
        size_t capacity;                                                        \
        size_t element_size;                                                    \
        T * data;                                                               \
    }
typedef array_t(uint8_t) array_base_t;

#define ARRAY_CAST(array) ((array_base_t *) array)

#define array_new(T) ((T) {                                                     \
    .size = 0,                                                                  \
    .capacity = 0,                                                              \
    .element_size = sizeof(((T *) 0)->data[0]),                                 \
    .data = NULL                                                                \
})

#define array_clear(array) array_base_clear(ARRAY_CAST(array))
void array_base_clear(array_base_t * array);

#define array_is_empty(array) array_base_is_empty(ARRAY_CAST(array))
bool array_base_is_empty(array_base_t * array);

#define array_front(array) (array->data)

#define array_back(array) (                                                     \
    (typeof(array->data)) array_base_back(ARRAY_CAST(array))                    \
)
uint8_t * array_base_back(array_base_t * array);

#define array_push_back(array, x) do {                                          \
    typeof(array) array_copy = (array);                                         \
    array_base_push_back(ARRAY_CAST(array_copy));                               \
    *array_back(array_copy) = (x);                                              \
} while (0)
void array_base_push_back(array_base_t * array);

#define array_pop_back(array) array_base_pop_back(ARRAY_CAST(array))
void array_base_pop_back(array_base_t * array);

#endif