#ifndef URIEL_ROUTER_H
#define URIEL_ROUTER_H

#include "server.h"

void uriel_router(const http_request_t * request, void * db, FILE * output);

#endif