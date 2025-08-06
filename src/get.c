#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

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

char* generate_page(char *content) {
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

    if (strcmp(req->path, "/") == 0) {
        content = generate_page("<h1 class=\"text-2xl\">Home Page</h1><p>lorem ipsum</p>");
    } else if (strcmp(req->path, "/about") == 0) {
        content = generate_page("<h1 class=\"text-2xl\">About Page</h1><p>lorem ipsum</p>");
    } else if (strcmp(req->path, "/signup") == 0) {
        //serve signup
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