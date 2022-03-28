#include "http.h"

int main(int c, char** v)
{
    http_serve_forever("8080");
    return 0;
}
