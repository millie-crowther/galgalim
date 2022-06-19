#ifndef ROUTER_H
#define ROUTER_H

#include <stdio.h>

#include "server.h"

int router_load_files();
void route(http_request_t * request, FILE * output);

#endif