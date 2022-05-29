#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <stdbool.h>

#include <hiredis.h>

bool instance_exists(redisContext * redis_context, const char * instance_id);

#endif 