#include "http.h"
#include "random.h"
#include <stdio.h>

int main(){
    random_t random = random_new();
    uuid_t uuid;
    random_uuid(&random, &uuid);
    random_destroy(&random);
    
    char string[UUID_STRING_LENGTH];
    uuid_to_string(&uuid, string);
    printf("uuid = %.*s\n", UUID_STRING_LENGTH, string); 
    printf("asdf\n");   
    printf("asdf\n");   
    
    http_serve_forever("8080");
    return 0;
}
