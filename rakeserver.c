#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "lib.h"

#define SERVER_PORT 5555

void set_nonblock(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1) {
        printf("get fcntl flag %s\n", strerror(errno));
        return;
    }
    int ret = fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    if (ret == -1) {
        printf("set fcntl non-blocking %s\n", strerror(errno));
        return;
    }
}

int socket_create(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        printf("socket create %s\n", strerror(errno));
        return -1;
    }
    set_nonblock(fd);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        printf("socket bind %s\n", strerror(errno));
        return -1;
    }
    if (listen(fd, 20) == -1) {
        printf("socket listen %s\n", strerror(errno));
        return -1;
    }
    return fd;
}

int accept_client(int epollfd, int server_fd) {
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    int len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
    if (client_fd == -1) {
        printf("socket accept %s\n", strerror(errno));
        return -1;
    }
    set_nonblock(client_fd);
    return client_fd;
}

int handelRequest(struct Transport *transport) {
    if (transport->method == QUERY) {

    }else if (transport->method == EXEC) {

    }
}

int main() {
    int server_fd = socket_create(SERVER_PORT);
    if (server_fd == -1) {
        printf("socket create server_fd failed\n");
        return errno;
    }
    printf("Listening on port %d\n", SERVER_PORT);
    int epfd = epoll_create(MAX_FD_NUM);
    if (epfd == -1) {
        printf("epoll create %s\n", strerror(errno));
        return errno;
    }
    struct epoll_event events[MAX_FD_NUM];
    bzero(events, sizeof(events));
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        printf("Add event failed!\n");
    }

    int client_fd;
    while (1) {
        int num = epoll_wait(epfd, events, MAX_FD_NUM, -1);
        if (num == -1) {
            printf("epoll wait %s\n", strerror(errno));
            break;
        } else {
            for (int i = 0; i<num; ++i) {
                if (events[i].data.fd == server_fd) {
                    struct Transport transport;
                    bzero(&transport, sizeof(transport));
                    client_fd = accept_client(epfd, server_fd);
                    if(client_fd == -1) {
                        continue;
                    }
                    transport.server_fd = client_fd;
                    transport.method = EMPTY;
                    //client_fd read server_fd client
                    server_add_event(epfd, &transport, EPOLLIN);
                    continue;
                } else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) {
                    printf("epoll err %s\n", strerror(errno));
                    struct Transport *transport = events[i].data.ptr;
                    server_delete_event(epfd, transport, events[i].events);
                    continue;
                } else if(events[i].events & EPOLLIN) {
                    //read msg
                    struct Transport *transport = events[i].data.ptr;
                    if(server_read(epfd, transport) <= 0) {
                        continue;
                    }
                    printf("%d\n", transport->method);
                    printf("%d\n", transport->client_fd);
                    printf("%s\n", transport->data);
                    //query action costs
                    if (transport->method == QUERY) {
                        double cost = getCost();
                        transport->method = COST;
                        sprintf(transport->data, "%.2f", cost);
                    }else if (transport->method == EXEC) {

                    }else {
                        transport->method = BAD_REQUEST;
                        bzero(transport->data, DATA_SIZE);
                        printf("Bad Request: %d\n", transport->method);
                    }
                } else if(events[i].events & EPOLLOUT){
                    struct Transport *transport = events[i].data.ptr;
                    if(transport != NULL) {
                        server_write(epfd, transport);
                    }else{
                        printf("Error!!!\n");
                    }
                }
            }
        }
    }
    return 0;
}

