#include "server.h"

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
#include <pthread.h>

#include "array.h"
#include "file.h"
#include "random.h"
#include "string.h"

#define BUFFER_SIZE 65536

void * http_handle_request_thread_function(void * data){
    http_request_t * request = (http_request_t *) data;
    char * buffer = calloc(BUFFER_SIZE + 1, 1);
    int received_bytes = recv(request->clientfd, buffer, BUFFER_SIZE, 0);
    buffer[received_bytes] = '\0';

    if (received_bytes < 0){
        fprintf(stderr, "Error receiving data from socket.\n");

    } else if (received_bytes == 0){    
        fprintf(stderr, "Client disconnected unexpectedly.\n");

    } else {
        http_build_request(request, buffer);

        FILE * output = fdopen(request->clientfd, "w");
        if (output != NULL){
            route(request, output);
            fflush(output);
            shutdown(STDOUT_FILENO, SHUT_WR);
            close(STDOUT_FILENO);
        }
    }

    shutdown(request->clientfd, SHUT_RDWR); 
    close(request->clientfd);
    free(request);
    return NULL;
}

void http_serve_forever(const char * port, redisContext * redis_context){
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
            fprintf(stderr, "Failed to accept socket connection.\n");
            continue;
        } 

        http_request_t * request = malloc(sizeof(http_request_t));
        if (request == NULL){
            fprintf(stderr, "Failed to allocate memory for request object.\n");
            continue;
        }

        *request = (http_request_t) {
            .clientfd = clientfd,
            .redis_context = redis_context
        };

        int result = pthread_create(&request->thread, NULL, http_handle_request_thread_function, (void *) request);
        if (result != 0){
            fprintf(stderr, "Failed to create thread to handle request.\n");
            free(request);
            continue;
        }

        pthread_detach(request->thread);
    }
}

int http_start_listening(const char *port){
    struct addrinfo hints, *addresses, *address_pointer;

    if (router_load_files() != 0){
        fprintf(stderr, "Error loading static content\n");
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