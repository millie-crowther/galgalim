#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char* argv[]) {
    /* Setup and connect to cluster */
    CassCluster * cluster = cass_cluster_new();
    CassSession * session = cass_session_new();

     /* Setup driver to connect to the cloud using the secure connection bundle */
     const char * secure_connect_bundle = "/secure-connect-galgalim-db.zip";
     if (cass_cluster_set_cloud_secure_connection_bundle(cluster, secure_connect_bundle) != CASS_OK) {
         fprintf(stderr, "Unable to configure cloud using the secure connection bundle: %s\n",
                 secure_connect_bundle);
         return 1;
     }
     

     /* Set credentials provided when creating your database */
     cass_cluster_set_credentials(cluster, "<<CLIENT ID>>", "<<CLIENT SECRET>>");

     /* Increase the connection timeout */
     cass_cluster_set_connect_timeout(cluster, 10000);

     CassFuture* connect_future = cass_session_connect(session, cluster);

     if (cass_future_error_code(connect_future) == CASS_OK) {

         /* Use the session to run queries */
         /* Build statement and execute query */

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

     } else {
         /* Handle error */
     }

     cass_future_free(connect_future);
     cass_cluster_free(cluster);
     cass_session_free(session);

     return 0;

 }

