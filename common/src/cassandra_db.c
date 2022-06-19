#include "cassandra_db.h"

#include <stdlib.h>
#include <stdio.h>

#include "string.h"

void print_error(CassFuture * future) {
    const char * message;
    size_t message_length;
    cass_future_error_message(future, &message, &message_length);
    fprintf(stderr, "Error: %.*s\n", (int) message_length, message);
}

CassCluster * db_create_cluster(){
    const char * client_id = getenv("CASSANDRA_CLIENT_ID");
    const char * client_secret = getenv("CASSANDRA_CLIENT_SECRET");

    if (client_id == NULL || string_equals(client_id, "") || client_secret == NULL || string_equals(client_secret, "")){
        fprintf(stderr, "Failed to read authetication credentials from environment.\n");
        return NULL;
    }

    CassCluster* cluster = cass_cluster_new();
    const char* secure_connect_bundle = "/secure-connect-galgalim-db.zip";
    if (cass_cluster_set_cloud_secure_connection_bundle(cluster, secure_connect_bundle) != CASS_OK) {
        fprintf(stderr, "Unable to configure cloud using the secure connection bundle: %s\n", secure_connect_bundle);
        return NULL;
    }
    cass_cluster_set_credentials(cluster, client_id, client_secret);    

    cass_cluster_set_connect_timeout(cluster, 10000);
    return cluster;
}

CassSession * db_create_session(CassCluster * cluster){
    CassSession * session = cass_session_new();
    CassFuture * future = cass_session_connect(session, cluster);
    cass_future_wait(future);
    if (cass_future_error_code(future) != CASS_OK){
        print_error(future);
        cass_session_free(session);
        return NULL;
    }

    cass_future_free(future);
    return session;
}