#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cassandra_db.h"
#include "server.h"
#include "router.h"

int main(int argc, char * argv[]) {
    CassCluster * cluster = db_create_cluster();
    if (cluster == NULL){
        exit(1);
    }

    CassSession * session = db_create_session(cluster);
    if (session == NULL){
        cass_cluster_free(cluster);
        exit(2);
    }

    http_serve_forever("80", (void *) session, uriel_router);

    cass_session_free(session);
    cass_cluster_free(cluster);

    return 0;
 }

