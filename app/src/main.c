#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"

int main() {
    redisContext * redis_context;
    const char * hostname = "host.docker.internal";
    int redis_port = 6379;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    redis_context = redisConnectWithTimeout(hostname, redis_port, timeout);
    if (redis_context == NULL || redis_context->err) {
        if (redis_context) {
            printf("Connection error: %s\n", redis_context->errstr);
            redisFree(redis_context);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    redisCommand(redis_context, "FLUSHDB");

    http_serve_forever("8080", redis_context);
    redisFree(redis_context);
    return 0;
}
