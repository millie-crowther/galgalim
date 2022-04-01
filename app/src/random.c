#include "random.h"

void random_seed(random_t * random, uint64_t seed, uint64_t sequence){
    random->state = 0U;
    random->inc = (sequence << 1u) | 1u;
    random_u32(random);
    random->state += seed;
    random_u32(random);
}

uint32_t random_u32(random_t * random){
    uint64_t oldstate = random->state;
    random->state = oldstate * 6364136223846793005ULL + random->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}