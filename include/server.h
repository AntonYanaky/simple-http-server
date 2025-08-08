#ifndef SERVER_H_
#define SERVER_H_

//struct to hold all the key value pairs that come after the first line of the header
struct key_value {
    char *key;
    char *value;
    struct key_value *next;
};

struct client_request {
    char *method;
    char *path;
    char *version;
    struct key_value *headers;
};

int handle_get_request(int client_fd, struct client_request *req);

#endif
