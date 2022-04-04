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
#include "file.h"
#include "string.h"

#define BUFFER_SIZE 65536

char * homepage_html;
char * frontend_js;

void route(http_request_t * request){
    if (string_starts_with(request->request_line_string, string_new("GET / "))){
        printf("HTTP/1.1 200 OK\r\n\r\n%s", homepage_html);
        return; 
    }
    
    if (string_starts_with(request->request_line_string, string_new("GET /frontend.js "))){
        printf("HTTP/1.1 200 OK\r\nContent-Type:text/javascript\r\n\r\n%s", frontend_js);
        return; 
    }

    if (string_starts_with(request->request_line_string, string_new("POST /event "))){
        bool error = false;
        json_t json = json_load(request->payload, request->payload_length);
        error |= json.type == JSON_TYPE_ERROR;
        json_t event_name = json_dictionary_find_key(json, string_new("event_name"));
        error |= event_name.type == JSON_TYPE_ERROR;
        fprintf(stderr, "json = %s\nevent name = %s\n", json.buffer, event_name.buffer);

        if (error){
            printf("HTTP/1.1 422 Unprocessable Entity\r\n\r\n");
            return;
        }

        printf("HTTP/1.1 202 Accepted\r\n\r\n");
        return;
    }
    
    printf("HTTP/1.1 404 Not Found\r\n\r\nThe requested page was not found.\r\n");
}

void http_close_socket(const int file_descriptor){
    shutdown(file_descriptor, SHUT_RDWR); 
    close(file_descriptor);
}

void http_serve_forever(const char * port){
    struct sockaddr_in client_address;
    socklen_t address_length;
    int clientfd;

    int listenfd = http_start_listening(port);  
    if (listenfd == -1){
        exit(1);
    }

    while (1){
        address_length = sizeof(client_address);
        clientfd = accept(listenfd, (struct sockaddr *) &client_address, &address_length);

        if (clientfd < 0){
            fprintf(stderr, "Error accepting socket connection.\n");

        } else {
            pid_t process_id = fork();
            if (process_id < 0){
                fprintf(stderr, "Error forking new process to handle request.\n");
                http_close_socket(clientfd);

            } else if (process_id == 0){
                char * buffer = calloc(BUFFER_SIZE + 1, 1);
                int received_bytes = recv(clientfd, buffer, BUFFER_SIZE, 0);

                if (received_bytes < 0){
                    fprintf(stderr, "Error receiving data from socket.\n");

                } else if (received_bytes == 0){    
                    fprintf(stderr, "Client disconnected unexpectedly.\n");

                } else {
                    http_request_t request;
                    http_build_request(&request, buffer, received_bytes);

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

    homepage_html = file_read("/static/homepage.html");
    if (homepage_html == NULL){
        fprintf(stderr, "Error loading HTML for homepage.\n");
        return -1;
    }

    frontend_js = file_read("/static/frontend.js");
    if (frontend_js == NULL){
        fprintf(stderr, "Error loading JavaScript for frontend.\n");
        return -1;
    }

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

void http_build_request(http_request_t * request, char * buffer, uint32_t length){
    // see RFC 2616, Section 5.1
    // https://www.rfc-editor.org/rfc/rfc2616#section-5.1
    string_t payload = (string_t) {
        .chars = buffer,
        .size = length
    };
    
    string_split(payload, string_new("\r\n"), &request->request_line_string, &payload);
    string_split(request->request_line_string, string_new(" "), &request->method, &request->uri);
    string_split(request->uri, string_new(" "), &request->uri, &request->protocol);
    string_split(request->uri, string_new("?"), &request->uri, &request->query_parameters);
    string_split(payload, string_new("\r\n\r\n"), &request->headers, &payload);
    request->headers.chars -= 2;
    request->headers.size += 2;
    request->payload = strstr(buffer, "\r\n\r\n") + 4;
    request->payload_length = length - (request->payload - buffer);

    fprintf(stderr, "\x1b[32m + [%.*s] %.*s\x1b[0m\n", request->method.size, request->method.chars, request->uri.size, request->uri.chars);

    string_t content_length_string = http_header_value(request, string_new("Content-Length"));
    long content_length = atol(content_length_string.chars);
    if (content_length != 0 && content_length < length){
        request->payload_length = content_length;
    }
}

string_t http_header_value(const http_request_t * request, const string_t header_name){
    char key[header_name.size + 5];
    sprintf(key, "\r\n%.*s: ", header_name.size, header_name.chars);
    string_t _;
    string_t header_value;
    string_split(request->headers, string_new(key), &_, &header_value);
    string_split(header_value, string_new("\r\n"), &header_value, &_);
    return header_value;
}