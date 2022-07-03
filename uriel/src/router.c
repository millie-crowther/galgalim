#include "router.h"

#include <cassandra.h>

#include "string.h"
#include <string.h>

void uriel_router(const http_request_t * request, void * db, FILE * output){
    CassSession * session = (CassSession *) db; 

    if (string_starts_with(request->uri, "/image/")){
        if (!string_equals(request->method, "GET")){
            fprintf(output, "HTTP/1.1 405 Method Not Allowed\r\n\r\n");
            return;
        }

        const char * query = "select uri from uriel_ks.image where id = ?";
        CassStatement * statement = cass_statement_new(query, 1);
        CassUuid uuid;
        const char * uri_uuid = request->uri + strlen("/image/");
        CassError is_uuid = cass_uuid_from_string(uri_uuid, &uuid);
        if (is_uuid != CASS_OK){
            fprintf(output, "HTTP/1.1 404 Not Found\r\n\r\nUnable to find image with id '%s'", uri_uuid);
            cass_statement_free(statement);
            return;
        }
        cass_statement_bind_uuid(statement, 0, uuid);

        CassFuture * result_future = cass_session_execute(session, statement);

        if (cass_future_error_code(result_future) == CASS_OK) {
            /* Retrieve result set and get the first row */
            const CassResult * result = cass_future_get_result(result_future);
            const CassRow * row = cass_result_first_row(result);

            if (row) {
                const CassValue * value = cass_row_get_column_by_name(row, "uri");
                const char * uri;
                size_t uri_length;
                cass_value_get_string(value, &uri, &uri_length);
                fprintf(output, "HTTP/1.1 200 OK\r\n\r\n{\"uri\":\"%.*s\"}\r\n", (int) uri_length, uri);
            } else {
                fprintf(output, "HTTP/1.1 404 Not Found\r\n\r\nUnable to find image with id '%s'", uri_uuid);
            }

            cass_result_free(result);
        } else {
            const char * message;
            size_t message_length;
            cass_future_error_message(result_future, &message, &message_length);
            fprintf(stderr, "Unable to run query: '%.*s'\n", (int) message_length, message);
            fprintf(output, "HTTP/1.1 500 Internal Server Error\r\n\r\n");
        }

        cass_future_free(result_future);
        cass_statement_free(statement);
    }
}