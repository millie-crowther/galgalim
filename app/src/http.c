#include "http.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#include "array.h"
#include "string.h"

// TODO: check maximum connections
#define MAXIMUM_CONNECTIONS 1000
#define BUFFER_SIZE 65536

const char * http_status_codes[HTTP_STATUS_END] = {
    [HTTP_STATUS_OK] = "200 OK",
    [HTTP_STATUS_BAD_REQUEST] = "400 Bad Request",
    [HTTP_STATUS_NOT_FOUND] = "404 Not Found",
    [HTTP_STATUS_INTERNAL_SERVER_ERROR] = "500 Internal Server Error"
};

void route(http_request_t * request){
    if (string_equals(request->uri, string_literal("/index")) && string_equals(request->method, string_literal("GET"))){
        string_t user_agent = http_get_header_value(&request->headers, string_literal("User-Agent"));
        http_status_code(HTTP_STATUS_OK);
        printf("hello world\n");
        printf("Hello! You are using %.*s", user_agent.size, user_agent.chars);
        return; 
    }
    
    http_status_code(HTTP_STATUS_NOT_FOUND);
    printf("The requested page was not found.\r\n");
}

void http_close_socket(const int file_descriptor){
    shutdown(file_descriptor, SHUT_RDWR); 
    close(file_descriptor);
}

void http_serve_forever(const char * port){
    struct sockaddr_in client_address;
    socklen_t address_length;
    int clientfd;
    char * buffer;
    int received_bytes;

    int listenfd = http_start_listening(port);  
    if (listenfd == -1){
        exit(1);
    }

    int slot = 0;
    pid_t * process_ids = malloc(sizeof(int) * MAXIMUM_CONNECTIONS);
    for (int i = 0; i < MAXIMUM_CONNECTIONS; i++){
        process_ids[i] = -1;
    }

    while (1){
        slot = (slot + 1) % MAXIMUM_CONNECTIONS;

        // TODO: limit max connections by waiting on old child processes
        // if (process_ids[slot] >= 0){
        //     int status;
        //     int options;
        //     waitpid(process_ids[slot], &status, options);
        //     process_ids[slot] = -1;
        // }

        address_length = sizeof(client_address);
        clientfd = accept(listenfd, (struct sockaddr *) &client_address, &address_length);

        if (clientfd < 0){
            fprintf(stderr, "Error accepting socket connection.\n");

        } else {
            pid_t process_id = fork();
            if (process_id < 0){
                fprintf(stderr, "Error forking new process to handle request.\n");
                http_status_code(HTTP_STATUS_INTERNAL_SERVER_ERROR);
                http_close_socket(clientfd);

            } else if (process_id > 0){
                process_ids[slot] = process_id;

            } else {
                buffer = malloc(BUFFER_SIZE);
                received_bytes = recv(clientfd, buffer, BUFFER_SIZE, 0);

                if (received_bytes < 0){
                    fprintf(stderr, "Error receiving data from socket.\n");
                    http_status_code(HTTP_STATUS_INTERNAL_SERVER_ERROR);

                } else if (received_bytes == 0){    
                    fprintf(stderr, "Client disconnected unexpectedly.\n");
                    http_status_code(HTTP_STATUS_BAD_REQUEST);

                } else {
                    http_request_t request;
                    http_build_request(&request, (string_t) { .chars = buffer, .size = received_bytes });

                    dup2(clientfd, STDOUT_FILENO);
                    close(clientfd);

                    route(&request);

                    fflush(stdout);
                    shutdown(STDOUT_FILENO, SHUT_WR);
                    close(STDOUT_FILENO);
                }

                http_close_socket(clientfd);
                exit(0);
            }
        }
    }
}

int http_start_listening(const char *port){
    struct addrinfo hints, *addresses, *address_pointer;

    // getaddrinfo for host
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo( NULL, port, &hints, &addresses) != 0){
        fprintf(stderr, "getaddrinfo() error");
        return -1;
    }

    // socket and bind
    int listenfd;
    for (address_pointer = addresses; address_pointer != NULL; address_pointer = address_pointer->ai_next){
        int option = 1;
        listenfd = socket (address_pointer->ai_family, address_pointer->ai_socktype, 0);
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

        if (listenfd == -1){ 
            continue;
        }

        if (bind(listenfd, address_pointer->ai_addr, address_pointer->ai_addrlen) == 0){ 
            break;
        }
    }

    if (address_pointer == NULL){
        fprintf(stderr, "socket() or bind()");
        return -1;
    }

    freeaddrinfo(addresses);

    // listen for incoming connections
    if (listen(listenfd, 1000000) != 0){
        fprintf(stderr, "listen() error");
        return -1;
    }
    
    // Ignore SIGCHLD to avoid zombie threads
    signal(SIGCHLD, SIG_IGN);

    fprintf(stderr, "Server started %shttp://127.0.0.1:%s%s\n", "\033[92m", port, "\033[0m");

    return listenfd;
}

void http_build_request(http_request_t * request, const string_t buffer){
    string_t payload;

    // see RFC 2616, Section 5.1
    // https://www.rfc-editor.org/rfc/rfc2616#section-5.1
    string_t request_line_string;
    // TODO: check "\r\n" at end of line rather than just '\n'
    string_split(buffer, '\n', &request_line_string, &payload);
    request_line_string = string_strip(request_line_string);
    string_split(request_line_string, ' ', &request->method, &request_line_string);
    string_split(request_line_string, ' ', &request->uri, &request_line_string);
    string_split(request_line_string, ' ', &request->protocol, &request_line_string);
    string_split(request->uri, '?', &request->uri, &request->query_parameters);

    fprintf(stderr, "\x1b[32m + [%.*s] %.*s\x1b[0m\n", request->method.size, request->method.chars, request->uri.size, request->uri.chars);

    string_t header, header_name, header_value;
    request->headers = array_new(http_header_array_t);
    while (payload.size >= 2 && !string_starts_with(payload, string_literal("\r\n"))){
        // TODO: check "\r\n" at end of line rather than just '\n'
        string_split(payload, '\n', &header, &payload);
        string_split(header, ':', &header_name, &header_value);
        header_value = string_strip(header_value);
        array_push_back(&request->headers);
        request->headers.data[request->headers.size - 1] = (http_header_t){
            .name = header_name,
            .value = header_value
        };
    }
    payload.chars += 2;
    payload.size = payload.size < 2 ? 0 : payload.size - 2;

    string_t content_length_string = http_get_header_value(&request->headers, string_literal("Content-Length"));
    if (!string_equals(content_length_string, empty_string)){
        long content_length = atol(content_length_string.chars);
        if (content_length != 0 && content_length < payload.size){
            payload.size = content_length;
        }
    }
}

string_t http_get_header_value(http_header_array_t * headers, string_t name){
    for (int i = 0; i < headers->size; i++){
        if (string_equals(headers->data[i].name, name)){
            return headers->data[i].value;
        }
    }

    return empty_string;
}

void http_status_code(const http_status_t status){
    printf("HTTP/1.1 %s\r\n\r\n", http_status_codes[status]);
}