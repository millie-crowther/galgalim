#include "router.h"

#include <stdlib.h>

#include "file.h"
#include "random.h"

char * homepage_html;
char * game_html;
char * frontend_js;
char * vertex_glsl;
char * fragment_glsl;

int router_load_files(){
    char ** pages[] = {&homepage_html, &game_html, &frontend_js, &vertex_glsl, &fragment_glsl};
    const char * filenames[] = {"/static/homepage.html", "/static/game.html", "/static/frontend.js", "/static/vertex.glsl", "/static/fragment.glsl"};

    size_t _;
    for (uint32_t i = 0; i < sizeof(filenames) / sizeof(*filenames); i++){
        *pages[i] = file_read(filenames[i], &_);
        if (*pages[i] == NULL){
            return -1;
        }
    }

    return 0;
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
