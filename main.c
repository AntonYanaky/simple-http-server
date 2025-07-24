#define _POSIX_C_SOURCE 200809L //define this for some reason

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#define PORT "4111" //port users will connect to

#define BACKLOG 10 //max connections in queue

void print_server_urls(const char *port) {
    struct ifaddrs *ifaddr, *ifa;
    char ip[INET_ADDRSTRLEN];

    // get all interface data, prints error and returns if it fails
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    printf("Server is listening. Access it from other devices using:\n");

    //loop through each interface found
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        //if no address for this one, skip it
        if (ifa->ifa_addr == NULL) continue;

        //check if it's an IPv4 address
        if (ifa->ifa_addr->sa_family == AF_INET) {
            //get a pointer to the raw, binary IP address data
            void *addr_ptr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            
            //convert the binary IP to a normal string
            inet_ntop(AF_INET, addr_ptr, ip, INET_ADDRSTRLEN);

            if (strcmp(ip, "127.0.0.1") != 0) {
                printf(" -> On network '%s': http://%s:%s\n", ifa->ifa_name, ip, port);
            }
        }
    }

    //free the no longer needed list
    freeifaddrs(ifaddr);
    printf("------------------------------------------------------\n");
}

int main() {
    struct addrinfo hints, *res, *p;
    int listener;
    int yes = 1;
    int rv;

    //setup my own address information
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; //use either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; //find my ip for me

    //getaddrinfo call and error checking
    if((rv = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    }

    //loop through the linked list returned my getaddrinfo and bind to the first one
    for (p = res; p != NULL; p = p->ai_next) {
        //calls socket to create a socket with the current information, if failure occurs, moves to the next linked list item
        if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket creation error");
            continue;
        }

        //fix the annoying issue of "port in us" or whatever, i.e. this allows you to reuse the port on reruns of the program
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt error");
        }

        //bind socket to my address and port specified
        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
            close(listener);
            perror("binding error");
            continue;
        }

        //if none of the if statements trigger it means everything was successful, no need to iterate through the rest
        break;
    }

    //free the linked list
    freeaddrinfo(res); 

    //if none of the linked list options worked close the program
    if (p == NULL) {
        fprintf(stderr, "failed to bind\n");
        exit(1);
    }

    //starts listening, if listen function returns -1 (failure) exits
    if (listen(listener, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    print_server_urls(PORT);

    printf("server: waiting for connections...\n");

    //main server loop
    while(1) {
        struct sockaddr_storage client_addr;
        socklen_t sin_size = sizeof client_addr;

        //blocking function, will wait until someone tries to connect to the server and then move on, accept returns a new socket to communicate on
        int new_fd = accept(listener, (struct sockaddr *)&client_addr, &sin_size);

        //if something goes wrong with accept
        if (new_fd == -1) {
            perror("accept error");
            continue;
        }

        printf("server: connecting\n");

        char http_response[] = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/html; charset=UTF-8\r\n\r\n"
                               "<!DOCTYPE html>""<html><head><title>Modern C Server</title>"
                               "<style>body { font-family: sans-serif; background-color: #f0f0f0; }</style></head>"
                               "<body><h1>Hello from a Modern C Server!</h1>"
                               "<p>This server is made by me.</p></body></html>";

        //send the premade http response to the client, print error if something goes wrong
        if (send(new_fd, http_response, sizeof(http_response) - 1, 0) == -1) {
            perror("send error");
        }
        
        close(new_fd);
    }

    return 0;
}