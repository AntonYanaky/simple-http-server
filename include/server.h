//header file for shared functions and other stuff
#ifndef SERVER_H_
#define SERVER_H_

//struct to hold all the key value pairs that come after the first line of the header
struct key_value {
    char *key;
    char *value;
    struct key_value *next;
};

//struct to hold parsed information
struct client_request {
    char *method; //"GET" or others
    char *path; //url path like "/index.html"
    char *version; //http version
    struct key_value *headers; //list to all the other information
};

int handle_get_request(int client_fd, struct client_request *req);

#endif
