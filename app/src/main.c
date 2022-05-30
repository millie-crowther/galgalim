#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "database.h"
#include "server.h"
/*
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
*/
#include <cassandra.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    CassCluster * cluster = db_create_cluster();
    if (cluster == NULL){
        exit(1);
    }

    CassSession * session = db_create_session(cluster);
    if (session == NULL){
        cass_cluster_free(cluster);
        exit(2);
    }

    const char* query = "SELECT release_version FROM system.local";
    CassStatement* statement = cass_statement_new(query, 0);

    CassFuture* result_future = cass_session_execute(session, statement);

    if (cass_future_error_code(result_future) == CASS_OK) {
        /* Retrieve result set and get the first row */
        const CassResult* result = cass_future_get_result(result_future);
        const CassRow* row = cass_result_first_row(result);

        if (row) {
            const CassValue* value = cass_row_get_column_by_name(row, "release_version");

            const char* release_version;
            size_t release_version_length;
            cass_value_get_string(value, &release_version, &release_version_length);
            printf("release_version: '%.*s'\n", (int)release_version_length, release_version);
        }

        cass_result_free(result);
    } else {
        /* Handle error */
        const char* message;
        size_t message_length;
        cass_future_error_message(result_future, &message, &message_length);
        fprintf(stderr, "Unable to run query: '%.*s'\n", (int)message_length, message);
    }

    cass_statement_free(statement);
    cass_future_free(result_future);

    cass_session_free(session);
    cass_cluster_free(cluster);

    return 0;
 }

