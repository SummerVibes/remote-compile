#ifndef REMOTE_COMPILE_LIB_H
#define REMOTE_COMPILE_LIB_H

#define MAX_EPOLL_EVENTS 64
#define TIMEOUT -1
#define MAX_FD_NUM 100
#define BUF_SIZE 200
#define DATA_SIZE BUF_SIZE-12
#define MAX_HOSTS_NUM 10
#define MAX_ACTIONSET_NUM 10
#define MAX_HOSTS_NUM 10
#define ACTION_PREFIX_LENGTH 4
#define REQUIRE_PREFIX_LENGTH 8
#define MAX_FD_NUM 10
#define EPOLL_SIZE 1024
#define MAX_EPOLL_EVENTS 64
#define TIMEOUT -1

enum METHOD {
    EMPTY = 0,
    QUERY = 1,
    COST = 2,
    EXEC = 3,
    RESULT = 4,
    BAD_REQUEST = 5
};

struct Transport {
    enum METHOD method;
    int client_fd;
    int server_fd;
    char data[DATA_SIZE];
};

double getCost();

void client_add_event(int epollfd, struct Transport *transport, int state);
void server_add_event(int epollfd, struct Transport *transport, int state);

void client_delete_event(int epollfd, struct Transport *transport, int state);
void server_delete_event(int epollfd, struct Transport *transport, int state);

void client_modify_event(int epollfd, struct Transport *transport, int state);
void server_modify_event(int epollfd, struct Transport *transport, int state);

int client_read(int epollfd, struct Transport *transport);
int server_read(int epollfd, struct Transport *transport);
int client_write(int epollfd, struct Transport *transport);
int server_write(int epollfd, struct Transport *transport);

//void handle_events(int epollfd, struct epoll_event events[], int num, char *buf);

#endif //REMOTE_COMPILE_LIB_H
