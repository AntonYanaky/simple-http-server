#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

//helper function to replace instances of a sequence of characters with another
char *str_replace(const char *orig, const char *rep, const char *with) {
    //the string that will be returned
    char *result;
    //pointer
    const char *ins;
    //temp var used to build the result
    char *tmp;
    //length of the string to be replaced
    int len_rep; 
    //length of the replacement string
    int len_with;
    //the length of the segment before the next replacement
    int len_front;
    //total number of replacemenets to be made
    int count;

    //if the original string or string to be replaced are missing cant perform function
    if (!orig || !rep)
        return NULL;

    len_rep = strlen(rep);
    //if the string to be replaced is empty itll cause a loop
    if (len_rep == 0)
        return NULL; 

    //if replacement string is missing, treat is as an empty string
    if (!with)
        with = "";
    len_with = strlen(with);

    //count all occurances of rep
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    //allocate memory for the new string
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);
    if (!result)
        return NULL;

    //build the string
    while (count--) {
        //find the next occurrence of rep
        ins = strstr(orig, rep);
        //calculate the length of the part of the string before this
        len_front = ins - orig;
        
        //copy that front part into our result string
        tmp = strncpy(tmp, orig, len_front) + len_front;
        
        //copy the replacement string into our result string
        tmp = strcpy(tmp, with) + len_with;
        
        //move the original string pointer past the front part
        orig += len_front + len_rep; 
    }
    //after the loop, copy the rest of the original string
    strcpy(tmp, orig);
    
    return result;
}

char* generate_page(char *content) {
    //open base file assuming running from project root
    FILE *f = fopen("src/index.html", "rb");
    if (!f) {
        perror("Cannot open index.html");
        return NULL;
    }
    //moves cursor to end of file to read the size of file
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    //moves back to the front and reads the file
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

    //simple router
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
        //call free after done with content
        free(content);
    } else {
        //send 500 Internal Server Error
    }

    return 1;
}