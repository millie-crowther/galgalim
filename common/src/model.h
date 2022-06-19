#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <stdbool.h>

#include <cassandra.h>

bool instance_exists(CassSession * session, const char * game_id, const char * instance_id);

#endif 