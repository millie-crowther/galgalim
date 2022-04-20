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
char * game_html;
char * frontend_js;
char * vertex_glsl;
char * fragment_glsl;

void route(http_request_t * request){
    if (string_equals(request->method, "GET") && string_equals(request->uri, "/")){
        printf("HTTP/1.1 200 OK\r\n\r\n%s", homepage_html);
        return; 
    }

    if (string_equals(request->method, "GET") && string_equals(request->uri, "/game.html")){
        printf("HTTP/1.1 200 OK\r\n\r\n%s", game_html);
        return; 
    }
    
    if (string_equals(request->method, "GET") && string_equals(request->uri, "/frontend.js")){
        printf("HTTP/1.1 200 OK\r\nContent-Type:text/javascript\r\n\r\n%s", frontend_js);
        return; 
    }

    if (string_equals(request->method, "GET") && string_equals(request->uri, "/vertex.glsl")){
        printf("HTTP/1.1 200 OK\r\n\r\n%s", vertex_glsl);
        return; 
    }

    if (string_equals(request->method, "GET") && string_equals(request->uri, "/fragment.glsl")){
        printf("HTTP/1.1 200 OK\r\n\r\n%s", fragment_glsl);
        return; 
    }


    if (string_equals(request->method, "POST") && string_equals(request->uri, "/event")){
        bool error = false;
        json_t json = json_load(request->payload);
        error |= json_get_type(json) != JSON_TYPE_DICTIONARY;
        json_t event_name = json_dictionary_find_key(json, "name");
        error |= json_get_type(json) != JSON_TYPE_STRING;
        json_t type = json_dictionary_find_key(json, "type");
        error |= json_get_type(json) != JSON_TYPE_STRING;
        json_t key = json_dictionary_find_key(json, "key");
        error |= json_get_type(json) != JSON_TYPE_STRING;

        printf("name = %s\ntype = %s\nkey = %s\n", json_get_string(event_name), json_get_string(type), json_get_string(key));

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
                buffer[received_bytes] = '\0';

                if (received_bytes < 0){
                    fprintf(stderr, "Error receiving data from socket.\n");

                } else if (received_bytes == 0){    
                    fprintf(stderr, "Client disconnected unexpectedly.\n");

                } else {
                    http_request_t request;
                    http_build_request(&request, buffer);

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

    char ** pages[] = {&homepage_html, &game_html, &frontend_js, &vertex_glsl, &fragment_glsl};
    const char * filenames[] = {"/static/homepage.html", "/static/game.html", "/static/frontend.js", "/static/vertex.glsl", "/static/fragment.glsl"};

    for (uint32_t i = 0; i < sizeof(filenames) / sizeof(*filenames); i++){
        *pages[i] = file_read(filenames[i]);
        if (*pages[i] == NULL){
            fprintf(stderr, "Error loading static content\n");
            return -1;
        }
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

void http_build_request(http_request_t * request, char * buffer){
    // see RFC 2616, Section 5.1
    // https://www.rfc-editor.org/rfc/rfc2616#section-5.1
    request->payload = string_split(buffer, "\r\n\r\n");
    request->headers = string_split(buffer, "\r\n");
    request->method = buffer;
    request->uri = string_split(buffer, " ");
    request->protocol = string_split(request->uri, " ");
    request->query_parameters = string_split(request->uri, "?");

    fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m\n", request->method, request->uri);
}