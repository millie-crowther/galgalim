#include "http.h"
#include "file.h"
#include <stdio.h>
#include <string.h>

int main(){
    char json_string[] = "{ \"a\": 1, \"b\": 2, \"c\": {\"d\": 3 }, \"d\": 4, \"millie\": \t\"happy soon!\" }";
    printf("json = %s\n", json_string);
    json_t json = json_load(json_string, strlen(json_string));
    printf("whitespace stripped = %s\n", json.buffer);
    json_t millie = json_dictionary_find_key(json, string_new("millie"));
    printf("millie = %s\n", millie.buffer);
    string_t millie_string = json_get_string(millie);
    printf("millie is %.*s\n", millie_string.size, millie_string.chars);
    fflush(stdout);

    http_serve_forever("8080");
    return 0;
}
