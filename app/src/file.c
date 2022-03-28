#include "file.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char * file_read(const char * filename){
    const char * path = "/static/";
    char static_filename[strlen(path) + strlen(filename) + 1];
    static_filename[0] = '\0';
    strcat(static_filename, path);
    strcat(static_filename, filename);

    FILE * file = fopen(static_filename, "r");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char * string = (char *) malloc(size + 1);
    fread(string, 1, size, file);
    string[size] = '\0';

    fclose(file);

    return string;
}