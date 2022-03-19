#include "httpd.h"

int main(int c, char** v)
{
    serve_forever("8080");
    return 0;
}
