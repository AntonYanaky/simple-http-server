#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"
#include "framework.h"

char *str_replace(const char *orig, const char *rep, const char *with) {
    char *result;
    const char *ins;
    char *tmp;
    int len_rep, len_with, len_front, count;

    if (!orig || !rep) return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0) return NULL; 
    if (!with) with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);
    if (!result) return NULL;

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

char* generate_main_page() {
    FILE *f = fopen("src/index.html", "rb");
    if (!f) {
        perror("Cannot open src/index.html");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *template_string = malloc(fsize + 1);
    if (!template_string) {
        perror("malloc for template failed");
        fclose(f);
        return NULL;
    }
    
    size_t bytes_read = fread(template_string, 1, fsize, f);
    fclose(f);

    if (bytes_read != (size_t)fsize) {
        perror("Failed to read entire template file");
        free(template_string);
        return NULL;
    }
    template_string[fsize] = '\0';

    node *page_container = createNode("div", "", (attribute[]){
        {.key = "class", .value = "container mx-auto max-w-4xl p-8 bg-white shadow-md rounded-lg mt-10"}
    }, 1);

    node *title_node = createNode("h1", "Simple C Web Framework", (attribute[]){
        {.key = "class", .value = "text-4xl font-bold text-gray-800 mb-4 border-b pb-2"}
    }, 1);
    createChild(page_container, title_node);

    node *desc_node = createNode("p", "This is a demonstration of a web framework written entirely in C. It uses a tree-based node structure to build dynamic HTML pages on the server. The framework includes functions for creating nodes with attributes, adding child nodes, and rendering the entire tree to a valid HTML string.", (attribute[]){
        {.key = "class", .value = "text-lg text-gray-600 mb-8"}
    }, 1);
    createChild(page_container, desc_node);
    
    node *example_title = createNode("h2", "Code Example", (attribute[]){
        {.key = "class", .value = "text-2xl font-semibold text-gray-700 mb-3"}
    }, 1);
    createChild(page_container, example_title);

    const char* code_example_string =
        "// Create a simple paragraph node\n"
        "node *p = createNode(\"p\", \"This is a paragraph.\", NULL, 0);\n\n"
        "// Create a link with attributes\n"
        "node *a = createNode(\n"
        "    \"a\",\n"
        "    \"Click here\",\n"
        "    (attribute[]){\n"
        "        {.key = \"href\", .value = \"/about\"},\n"
        "    },\n"
        "    2\n"
        ");";

    node *code_block_container = createNode("div", "", (attribute[]){
        {.key = "class", .value = "bg-gray-800 text-white p-4 rounded-md overflow-x-auto"}
    }, 1);
    node *pre_node = createNode("pre", "", NULL, 0);
    node *code_node = createNode("code", code_example_string, (attribute[]){
        {.key = "class", .value = "language-c"}
    }, 1);

    createChild(pre_node, code_node);
    createChild(code_block_container, pre_node);
    createChild(page_container, code_block_container);

    char *content_html = renderNodes(page_container);
    freeNodes(page_container);

    char *page_html = str_replace(template_string, "{{PAGE_CONTENT}}", content_html);
    free(template_string);
    free(content_html);

    if (!page_html) {
        perror("str_replace failed, resulting in NULL page_html");
        return NULL;
    }

    const char *status = "HTTP/1.1 200 OK\r\n";
    const char *header = "Content-Type: text/html; charset=UTF-8\r\n\r\n";
    size_t total_len = strlen(status) + strlen(header) + strlen(page_html) + 1;
    char *response = malloc(total_len);
    
    if (!response) {
        free(page_html);
        return NULL;
    }
    
    strcpy(response, status);
    strcat(response, header);
    strcat(response, page_html);

    free(page_html);
    return response;
}

int handle_get_request(int client_fd, struct client_request *req) {
    if (strcmp(req->path, "/") == 0) {
        char* content = generate_main_page();
        if (content != NULL) {
            send(client_fd, content, strlen(content), 0);
            free(content);
        }
        return 1;
    }
    
    char* response_404 = "HTTP/1.1 404 Not Found\r\n\r\nNot Found";
    send(client_fd, response_404, strlen(response_404), 0);
    return 1;
}
