#ifndef DATABASE_H
#define DATABASE_H

#include <cassandra.h>

CassCluster * db_create_cluster();
CassSession * db_create_session(CassCluster * cluster);

#endif