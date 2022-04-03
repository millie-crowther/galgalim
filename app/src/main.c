#include "http.h"
#include "file.h"
#include <stdio.h>
#include <string.h>

int main(){
    char json_string[] = "{ \"a\": 1, \"b\": 2, \"c\": {\"d\": 3 }, \"d\": 4 }";
    printf("json = %s\n", json_string);
    json_t json = json_load(json_string, strlen(json_string));
    printf("whitespace stripped = %s\n", json.buffer);
    json_t a = json_dictionary_find_key(json, string_new("a"));
    json_t b = json_dictionary_find_key(json, string_new("b"));
    json_t d = json_dictionary_find_key(json, string_new("d"));
    printf("a = %s\nb = %s\nd = %s\n", a.buffer, b.buffer, d.buffer);
    fflush(stdout);

    http_serve_forever("8080");
    return 0;
}
