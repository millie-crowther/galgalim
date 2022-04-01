#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

typedef struct random_t {
    uint64_t state;   
    uint64_t inc; 
} random_t;

void random_seed(random_t * random, uint64_t seed, uint64_t sequence);
uint32_t random_u32(random_t * random);

#endif