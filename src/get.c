#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"
#include "framework.h"

void serve_static_file(int client_fd, const char* file_path) {
    printf("Attempting to open static file at: \"%s\"\n", file_path);

    FILE *f = fopen(file_path, "rb");
    if (!f) {
        perror("fopen failed"); 
        char* response_404 = "HTTP/1.1 404 Not Found\r\n\r\nNot Found";
        send(client_fd, response_404, strlen(response_404), 0);
        return;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *file_content = malloc(fsize);
    if (!file_content) {
        fclose(f);
        return;
    }

    size_t bytes_read = fread(file_content, 1, fsize, f);
    if (bytes_read != (size_t)fsize) {
        perror("Failed to read entire static file");
        free(file_content);
        fclose(f);
        return;
    }
    fclose(f);

    const char *content_type = "text/plain";
    if (strstr(file_path, ".js")) {
        content_type = "application/javascript";
    } else if (strstr(file_path, ".css")) {
        content_type = "text/css";
    }

    char header_buffer[512];
    int header_len = snprintf(header_buffer, sizeof(header_buffer),
                              "HTTP/1.1 200 OK\r\n"
                              "Content-Type: %s; charset=UTF-8\r\n"
                              "Content-Length: %ld\r\n\r\n",
                              content_type, fsize);

    send(client_fd, header_buffer, header_len, 0);
    send(client_fd, file_content, fsize, 0);

    free(file_content);
}

char *str_replace(const char *orig, const char *rep, const char *with) {
    char *result;
    const char *ins;
    char *tmp;
    int len_rep; 
    int len_with;
    int len_front;
    int count;

    if (!orig || !rep)
        return NULL;

    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; 

    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);
    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        
        tmp = strncpy(tmp, orig, len_front) + len_front;
        
        tmp = strcpy(tmp, with) + len_with;
        
        orig += len_front + len_rep; 
    }
    strcpy(tmp, orig);
    
    return result;
}

char * generateTestPage() {
    FILE *f = fopen("src/index.html", "rb");
    if (!f) {
        perror("Cannot open index.html");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *template_string = malloc(fsize + 1);
    if (!template_string) {
        perror("malloc failed for template_string");
        fclose(f);
        return NULL;
    }
    size_t bytes_read = fread(template_string, 1, fsize, f);
    if (bytes_read != (size_t)fsize) {
        perror("Failed to read entire file");
        free(template_string);
        fclose(f);
        return NULL;
    }
    fclose(f);
    template_string[fsize] = '\0';

    node *form_container = createNode(
        "div",
        "",
        (attribute[]){
            {.key = "class", .value = "flex flex-col items-center space-y-4 p-8"}
        },
        1
    );

    node *lat_container = createNode("div", "", NULL, 0);

    node *lat_label = createNode(
        "label",
        "Latitude",
        (attribute[]){
            {.key = "for", .value = "latitude"},
            {.key = "class", .value = "block text-sm font-medium text-gray-700 mb-1"}
        },
        2
    );
    createChild(lat_container, lat_label);

    node *lat_input = createNode(
        "input",
        "",
        (attribute[]){
            {.key = "type", .value = "text"},
            {.key = "id", .value = "latitude"},
            {.key = "name", .value = "latitude"},
            {.key = "class", .value = "w-full border border-gray-300 px-3 py-2 text-gray-900 placeholder-gray-400 focus:outline-none focus:ring-2 focus:ring-slate-500"}
        },
        4
    );
    createChild(lat_container, lat_input);
    createChild(form_container, lat_container);


    node *lon_container = createNode("div", "", NULL, 0);

    node *lon_label = createNode(
        "label",
        "Longitude",
        (attribute[]){
            {.key = "for", .value = "longitude"},
            {.key = "class", .value = "block text-sm font-medium text-gray-700 mb-1"}
        },
        2
    );
    createChild(lon_container, lon_label);

    node *lon_input = createNode(
        "input",
        "",
        (attribute[]){
            {.key = "type", .value = "text"},
            {.key = "id", .value = "longitude"},
            {.key = "name", .value = "longitude"},
            {.key = "class", .value = "w-full rounded-md border border-gray-300 px-3 py-2 text-gray-900 placeholder-gray-400 focus:outline-none focus:ring-2 focus:ring-slate-500"}
        },
        4
    );
    createChild(lon_container, lon_input);
    createChild(form_container, lon_container);


    node *find_button = createNode(
        "a",
        "Get weather",
        (attribute[]){
            {.key = "id", .value = "weather-link"},
            {.key = "href", .value = "#"},
            {.key = "class", .value = "inline-block py-2 px-6 border border-transparent rounded-md shadow-sm text-sm font-medium text-white bg-emerald-400 hover:bg-emerald-600"},
            {.key = "role", .value = "button"}
        },
        4
    );
    createChild(form_container, find_button);



    char *content = renderNodes(form_container);

    if (content == NULL) {
        free(content);
        content = "erorr";
    }

    char *final_result = str_replace(template_string, "{{PAGE_CONTENT}}", content);
    free(template_string);
    if (!final_result) {
        perror("str_replace failed");
        return NULL;
    }

    //build full response
    const char *status = "HTTP/1.1 200 OK\r\n";
    const char *header = "Content-Type: text/html; charset=UTF-8\r\n\r\n";
    size_t total_len = strlen(status) + strlen(header) + strlen(final_result) + 1;
    char *response = malloc(total_len);
    if (!response) {
        perror("malloc failed for response");
        free(final_result);
        return NULL;
    }
    strcpy(response, status);
    strcat(response, header);
    strcat(response, final_result);

    free(final_result);
    return response;

}

char* generate_page(char *content) {
    generateTestPage();
    FILE *f = fopen("src/index.html", "rb");
    if (!f) {
        perror("Cannot open index.html");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *template_string = malloc(fsize + 1);
    if (!template_string) {
        perror("malloc failed for template_string");
        fclose(f);
        return NULL;
    }
    size_t bytes_read = fread(template_string, 1, fsize, f);
    if (bytes_read != (size_t)fsize) {
        perror("Failed to read entire file");
        free(template_string);
        fclose(f);
        return NULL;
    }
    fclose(f);
    template_string[fsize] = '\0';

    char *final_result = str_replace(template_string, "{{PAGE_CONTENT}}", content);
    free(template_string);
    if (!final_result) {
        perror("str_replace failed");
        return NULL;
    }

    //build full response
    const char *status = "HTTP/1.1 200 OK\r\n";
    const char *header = "Content-Type: text/html; charset=UTF-8\r\n\r\n";
    size_t total_len = strlen(status) + strlen(header) + strlen(final_result) + 1;
    char *response = malloc(total_len);
    if (!response) {
        perror("malloc failed for response");
        free(final_result);
        return NULL;
    }
    strcpy(response, status);
    strcat(response, header);
    strcat(response, final_result);

    free(final_result);
    return response;
}

int handle_get_request(int client_fd, struct client_request *req) {
    char* content = NULL;

    if (strncmp(req->path, "/src/", 5) == 0) {
        serve_static_file(client_fd, req->path + 1);
        return 1;
    }

    if (strcmp(req->path, "/") == 0) {
        content = generate_page("<h1 class=\"text-2xl\">Home Page</h1><p>lorem ipsum</p>");
    } else if (strcmp(req->path, "/about") == 0) {
        content = generate_page("<h1 class=\"text-2xl\">About Page</h1><p>lorem ipsum</p>");
    } else if (strcmp(req->path, "/test") == 0) {
        if (!(content = generateTestPage())) {
            perror("Something went wrong!");
            printf("Something went very wrong!");
        }
    } else {
        char* response_404 = "HTTP/1.1 404 Not Found\r\n\r\nNot Found";
        send(client_fd, response_404, strlen(response_404), 0);
        return 1;
    }
    
    if (content != NULL) {
        send(client_fd, content, strlen(content), 0);
        free(content);
    } else {
        //send 500 Internal Server Error
    }

    return 1;
}