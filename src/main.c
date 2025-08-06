//curently works on localhost:PORT and the local IPv4 Address:PORT
#define _POSIX_C_SOURCE 200809L //define this for some reason to have my vscode not throw a fit, program works without it.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <time.h>
#include <sys/timerfd.h>
#include "server.h"

#define BUFFER 8192
#define PORT "4111"
#define BACKLOG 128
#define MAX_EVENTS 64
#define MAX_CLIENTS 1024
#define TIMEOUT_SEC 1

struct client_state {
    int active;
    time_t last_activity;
};

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int is_keep_alive(struct client_request *req) {
    struct key_value *current = req->headers;
    while(current != NULL) {
        if (strcasecmp(current->key, "Connection") == 0) {
            if (strcasecmp(current->value, "keep-alive") == 0) {
                return 1; //keep alive
            }
            if (strcasecmp(current->value, "close") == 0) {
                return 0; //close
            }
        }
        current = current->next;
    }

    if (strcmp(req->version, "HTTP/1.1") == 0) {
        return 1;
    }
    
    return 0;
}

void free_client_request(struct client_request *req) {
    if (!req) return;
    struct key_value *current = req->headers;
    while (current != NULL) {
        struct key_value *next = current->next;
        free(current);
        current = next;
    }
    free(req);
}

int parse_request(char *request, int length, struct client_request *client_http) {
    client_http->headers = NULL;

    client_http->method = request;
    int i = 0;
    for (; request[i] != ' '; i++) ; //just iterates
    if (i == length) { free(client_http); return -1; } //error
    request[i++] = '\0';

    client_http->path = &request[i];
    for(; request[i] != ' '; i++) ;
    if (i == length) { free(client_http); return -1; }
    request[i++] = '\0';

    client_http->version = &request[i];
    for(; request[i] != '\r'; i++) ;
    if (i == length) { free(client_http); return -1; }
    request[i++] = '\0';

    if (i < length && request[i-1] == '\0' && request[i] == '\n') {
        i++;
    }

    printf("%s\n%s\n%s\n", client_http->method, client_http->path, client_http->version);

    struct key_value *head = NULL;
    struct key_value *tail = NULL;

    while (i < length && request[i] != '\r') {
        struct key_value *pair = (struct key_value *) malloc(sizeof(struct key_value));
        if (!pair) {
            free(client_http); 
            return -1;
        }
        pair->next = NULL;

        pair->key = &request[i];
        for (; request[i] != ':'; i++) ;
        if (i == length) { free(pair); free(client_http); return -1; } //error
        request[i++] = '\0';

        for (; request[i] == ' ' || request[i] == '\t'; i++) ;
        if (i == length) { free(pair); free(client_http); return -1; }

        pair->value = &request[i];
        for (; request[i] != '\r'; i++) ;
        if (i == length) { free(pair); free(client_http); return -1; }
        request[i++] = '\0';
        i++;

        if (head == NULL) {
            head = pair;
            tail = pair;
        } else {
            tail->next = pair;
            tail = pair;
        }
    }

    client_http->headers = head;

    //debug prints
    printf("Headers:\n");
    struct key_value *current = client_http->headers;
    while(current != NULL) {
        printf("  '%s': '%s'\n", current->key, current->value);
        current = current->next;
    }
    printf("\n");

    return 1;
}

int do_something(struct client_request *client_header, int socket) {
    if (strcmp(client_header->method, "GET") == 0) {
        handle_get_request(socket, client_header);
    } else if (strcmp(client_header->method, "POST") == 0) {
        //method for posting
    } else if (strcmp(client_header->method, "PUT") == 0) {
        //put method
    } else if (strcmp(client_header->method, "DELETE") == 0) {
        //delete method
    } else if (strcmp(client_header->method, "HEAD") == 0) {
        //sends back just the head, bassically GET but no body
    } else if (strcmp(client_header->method, "OPTIONS") == 0) {
        //options
    } else if (strcmp(client_header->method, "TRACE") == 0) {
        //trace
    } else if (strcmp(client_header->method, "CONNECT") == 0) {
        //connect
    } else if (strcmp(client_header->method, "PATCH") == 0) {
        //patch
    }

    return 1;
}

int main() {
    struct addrinfo hints, *res, *p;
    int listener;
    int yes = 1;
    int rv;

    //own address
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    }

    for (p = res; p != NULL; p = p->ai_next) {
        if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket creation error");
            continue;
        }

        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt error");
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            close(listener);
            perror("binding error");
            continue;
        }

        break;
    }

    freeaddrinfo(res); 

    if (p == NULL) {
        fprintf(stderr, "failed to bind\n");
        exit(1);
    }

    if (listen(listener, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    if (fcntl(listener, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl listener");
        exit(1);
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(1);
    }

    struct epoll_event event, events[MAX_EVENTS];
    event.events = EPOLLIN;
    event.data.fd = listener;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener, &event) == -1) {
        perror("epoll_ctl listener");
        exit(1);
    }

    //timer
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    struct itimerspec new_timer_value;
    new_timer_value.it_value.tv_sec = 1;
    new_timer_value.it_value.tv_nsec = 0;
    new_timer_value.it_interval.tv_sec = 1;
    new_timer_value.it_interval.tv_nsec = 0;
    timerfd_settime(timer_fd, 0, &new_timer_value, NULL);

    event.events = EPOLLIN;
    event.data.fd = timer_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &event);

    struct client_state clients[MAX_CLIENTS] = {0}; 
    char request_buffer[BUFFER];

    printf("server: waiting for connections...\n");


    while(1) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == listener) {
                while (1) {
                    int new_fd = accept(listener, NULL, NULL);
                    if (new_fd == -1) break;

                    fcntl(new_fd, F_SETFL, O_NONBLOCK);
                    event.events = EPOLLIN;
                    event.data.fd = new_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &event);
                    
                    if (new_fd < MAX_CLIENTS) {
                        clients[new_fd].active = 1;
                        clients[new_fd].last_activity = time(NULL);
                        printf("server: accepted new connection on fd %d\n", new_fd);
                    } else {
                        close(new_fd);
                    }
                }
            } else if (events[i].data.fd == timer_fd) {
                uint64_t expirations;
                if (read(timer_fd, &expirations, sizeof(uint64_t)) == -1) {
                    perror("read timer_fd");
                }

                time_t now = time(NULL);
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (clients[j].active && (now - clients[j].last_activity > TIMEOUT_SEC)) {
                        printf("server: closing idle connection on fd %d\n", j);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, j, NULL);
                        close(j);
                        clients[j].active = 0;
                    }
                }
            } else {
                int client_fd = events[i].data.fd;
                int bytes_received = recv(client_fd, request_buffer, BUFFER - 1, 0);

                if (bytes_received <= 0) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                    clients[client_fd].active = 0;
                } else {
                    request_buffer[bytes_received] = '\0';
                    struct client_request *client_http = malloc(sizeof(struct client_request));
                    
                    if (parse_request(request_buffer, bytes_received, client_http) > 0) {
                        do_something(client_http, client_fd);

                        if (is_keep_alive(client_http)) {
                            clients[client_fd].last_activity = time(NULL);
                            printf("server: keep-alive request processed on fd %d\n", client_fd);
                        } else {
                            printf("server: closing connection on fd %d as requested\n", client_fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                            close(client_fd);
                            clients[client_fd].active = 0;
                        }

                    }
                    free_client_request(client_http);
                }
            }
        }
    }

    close(listener);
    return 0;
}