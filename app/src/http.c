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
#include <pthread.h>

#include "array.h"
#include "file.h"
#include "random.h"
#include "string.h"

#define BUFFER_SIZE 65536

char * homepage_html;
char * game_html;
char * frontend_js;
char * vertex_glsl;
char * fragment_glsl;

bool instance_exists(redisContext * redis_context, const char * instance_id){
    redisReply * reply = redisCommand(redis_context, "SISMEMBER instances %s", instance_id);
    bool is_found = reply->integer;
    freeReplyObject(reply);
    return is_found;
}

bool route_instance(http_request_t * request, FILE * output){
    if (string_equals(request->method, "POST") && string_equals(request->uri, "/instance")){
        char instance_id[UUID_STRING_LENGTH];
        uuid_t uuid;
        random_t random = random_new();
        random_uuid(&random, &uuid);
        random_free(&random);
        uuid_to_string(&uuid, instance_id);
        redisReply * reply = redisCommand(request->redis_context, "SADD instances %s", instance_id);
        
        freeReplyObject(reply);
        fprintf(output, "HTTP/1.1 201 Created\r\n\r\n{\"ID\":\"%s\"}\r\n", instance_id);
        return true;

    } else if (string_equals(request->method, "GET") && string_starts_with(request->uri, "/instance/")){
        const char * instance_id = request->uri + strlen("/instance/");
        if (instance_exists(request->redis_context, instance_id)){
            fprintf(output, "HTTP/1.1 200 OK\r\n\r\n%s\r\n", game_html);
        } else {
            fprintf(output, "HTTP/1.1 404 Not Found\r\n\r\nUnable to find instance with id %s\r\n", instance_id);
        }
        return true; 
    }

    return false;
}

bool route_player(http_request_t * request, FILE * output){
    if (string_equals(request->method, "POST") && string_equals(request->uri, "/player")){
        json_t json = json_load(request->payload);
        bool is_error = json_get_type(json) != JSON_TYPE_DICTIONARY;
        json_t instance_id_json = json_dictionary_find_key(json, "instanceID");
        is_error |= json_get_type(instance_id_json) != JSON_TYPE_STRING;
        if (is_error){
            fprintf(output, "HTTP/1.1 422 Unprocessable Entity\r\n\r\nIncorrectly formatted JSON");
            return true;
        } 

        const char * instance_id = json_get_string(instance_id_json);
        if (!instance_exists(request->redis_context, instance_id)){
            fprintf(output, "HTTP/1.1 404 Not Found\r\n\r\nUnable to find instance with id %s\r\n", instance_id);
            return true; 
        }

        char player_id[UUID_STRING_LENGTH];
        uuid_t uuid;
        random_t random = random_new();
        random_uuid(&random, &uuid);
        random_free(&random);
        uuid_to_string(&uuid, player_id);
        redisReply * reply = redisCommand(request->redis_context, "SADD /instance/%s/players %s", instance_id, player_id);
        freeReplyObject(reply);
        fprintf(output, "HTTP/1.1 201 Created\r\n\r\n{\"instanceID\":\"%s\",\"ID\":\"%s\"}\r\n", instance_id, player_id);
        json_free(&json);
        return true;
    }

    return false;
}

void route(http_request_t * request, FILE * output){
    if (string_equals(request->method, "GET") && string_equals(request->uri, "/")){
        fprintf(output, "HTTP/1.1 200 OK\r\n\r\n%s", homepage_html);
        return; 
    }
    
    if (string_equals(request->method, "GET") && string_equals(request->uri, "/frontend.js")){
        fprintf(output, "HTTP/1.1 200 OK\r\nContent-Type:text/javascript\r\n\r\n%s", frontend_js);
        return; 
    }

    if (string_equals(request->method, "GET") && string_equals(request->uri, "/vertex.glsl")){
        fprintf(output, "HTTP/1.1 200 OK\r\n\r\n%s", vertex_glsl);
        return; 
    }

    if (string_equals(request->method, "GET") && string_equals(request->uri, "/fragment.glsl")){
        fprintf(output, "HTTP/1.1 200 OK\r\n\r\n%s", fragment_glsl);
        return; 
    }

    if (string_equals(request->method, "POST") && string_equals(request->uri, "/event")){
        fprintf(output, "HTTP/1.1 202 Accepted\r\n\r\n");
        return;
    }

    if (string_equals(request->method, "GET") && string_equals(request->uri, "/draco_gltf.js")){
        size_t size;
        char * draco_js = file_read("/static/draco_gltf.js", &size);
        if (draco_js == NULL){
            fprintf(output, "HTTP/1.1 500 Internal Server Error\r\nFailed to load draco library\r\n\r\n");
            return;
        }

        fprintf(output, "HTTP/1.1 200 OK\r\nContent-Type:text/javascript\r\n\r\n%s\r\n", draco_js);
        free(draco_js);
        return;
    }


    if (string_equals(request->method, "GET") && string_starts_with(request->uri, "/asset/")){
        if (string_contains(request->uri, "..")){
            fprintf(output, "HTTP/1.1 400 Bad Request\r\n\r\n");
            return;
        }

        size_t size;
        char * asset = file_read(request->uri, &size);
        if (asset == NULL){
            fprintf(output, "HTTP/1.1 404 Not Found\r\n\r\n");
            return;
        }

        fprintf(output, "HTTP/1.1 200 OK\r\n\r\n");
        fwrite(asset, 1, size, output);
        free(asset);
        return;
    }

    if (route_instance(request, output)){
        return;
    }

    if (route_player(request, output)){
        return;
    }

    fprintf(output, "HTTP/1.1 404 Not Found\r\n\r\nThe requested page was not found.\r\n");
}

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

    char ** pages[] = {&homepage_html, &game_html, &frontend_js, &vertex_glsl, &fragment_glsl};
    const char * filenames[] = {"/static/homepage.html", "/static/game.html", "/static/frontend.js", "/static/vertex.glsl", "/static/fragment.glsl"};

    size_t _;
    for (uint32_t i = 0; i < sizeof(filenames) / sizeof(*filenames); i++){
        *pages[i] = file_read(filenames[i], &_);
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