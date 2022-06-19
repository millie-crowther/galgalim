#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>
#include <stdio.h>

typedef struct random_t {
    FILE * file;
} random_t;

#define UUID_SIZE 16
#define UUID_STRING_LENGTH (UUID_SIZE * 2 + 1)
typedef struct uuid_t {
    uint8_t data[UUID_SIZE];
} uuid_t;

random_t random_new();
void random_free(random_t * random);
void random_uuid(random_t * random, uuid_t * uuid);
void uuid_to_string(uuid_t * uuid, char * string); 

#endif