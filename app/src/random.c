#include "random.h"
#include <stdio.h>

#include "file.h"

random_t random_new(){
    return (random_t){
        .file = fopen("/dev/urandom", "r")
    };
}

void random_destroy(random_t * random){
    if (random->file != NULL){
        fclose(random->file);
        random->file = NULL;
    }
}

void random_uuid(random_t * random, uuid_t * uuid){
    fread((void *) uuid, 1, sizeof(uuid_t), random->file);
}

void uuid_to_string(uuid_t * uuid, char * string){
    for (int i = 0; i < UUID_SIZE; i++){
        sprintf(string + i * 2, "%02hhx", uuid->data[i]);
    }
}