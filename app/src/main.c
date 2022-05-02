#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"

int main() {
    redisContext * redis_context;
    const char * hostname = "redis-18775.c78.eu-west-1-2.ec2.cloud.redislabs.com";
    int redis_port = 18775;

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

    redisReply * reply = redisCommand(redis_context, "AUTH BzBtjF4pCdpnF9DodD1Go8LzJGzj9f2x");
    if (!string_equals(reply->str, "OK")){
        printf("Failed to authorise for redis connections\n");
        redisFree(redis_context);
        exit(1);
    }
    freeReplyObject(reply);
    http_serve_forever("8080", redis_context);
    redisFree(redis_context);
    return 0;
}
