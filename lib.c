#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "lib.h"

double getCost()
{
    char result[1024] = {0};
    char buf[64] = {0};
    FILE *fp = NULL;

    if( (fp = popen("uptime", "r")) == NULL ) {
        printf("popen error!\n");
        return -1;
    }
    while (fgets(buf, sizeof(buf), fp)) {
        strcat(result, buf);
    }
    pclose(fp);
    char *idx = strrchr(result, ',');
    idx+=2;
    double res = atof(idx) * 100;
    printf("%.2f", res);
    return res;
}

void add_event(int epollfd, int sockfd, struct Transport *transport, int state) {
    struct epoll_event ev;
    ev.events = state;
    ev.data.ptr = transport;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        printf("Add event failed!\n");
    }
}

void server_add_event(int epollfd, struct Transport *transport, int state) {
    add_event(epollfd, transport->server_fd, transport, state);
}

void client_add_event(int epollfd, struct Transport *transport, int state) {
    add_event(epollfd, transport->client_fd, transport, state);
}



//This function will shut down the socket and free transport
void delete_event(int epollfd, int sockfd, struct Transport *transport, int state) {
    struct epoll_event ev;
    ev.events = state;
    ev.data.ptr = transport;
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, &ev) < 0) {
        printf("Delete event failed!\n");
    }
    //TODO free transport here
    close(sockfd);
//    free(transport);
}

void server_delete_event(int epollfd, struct Transport *transport, int state) {
    delete_event(epollfd, transport->server_fd, transport, state);
}

void client_delete_event(int epollfd, struct Transport *transport, int state) {
    delete_event(epollfd, transport->client_fd, transport, state);
}

void modify_event(int epollfd, int sockfd, struct Transport *transport,int state) {
    struct epoll_event ev;
    ev.events = state;
    ev.data.ptr = transport;
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, sockfd, &ev) < 0) {
        printf("Modify event failed!\n");
    }
}

void server_modify_event(int epollfd, struct Transport *transport, int state) {
    modify_event(epollfd, transport->server_fd, transport, state);
}

void client_modify_event(int epollfd, struct Transport *transport, int state) {
    modify_event(epollfd, transport->client_fd, transport, state);
}



int do_read(int epollfd, int sockfd, struct Transport *transport) {
    int nread;
    nread = read(sockfd, transport, BUF_SIZE);
    // transport->server_fd will be overwritten without this line, and we only need to back up this fd.
    transport->server_fd = sockfd;
    if (nread == -1) {
        perror("Read error:");
        delete_event(epollfd, sockfd, transport, EPOLLIN);
    }else if (nread == 0) {
        fprintf(stderr, "Server closed.\n");
        delete_event(epollfd, sockfd, transport, EPOLLIN);
    }else {
//        printf("Get msg : %s\n", buf);
        modify_event(epollfd, sockfd, transport, EPOLLOUT);
    }
    return nread;
}
int client_read(int epollfd, struct Transport *transport) {
    return do_read(epollfd, transport->client_fd, transport);
}

int server_read(int epollfd, struct Transport *transport) {
    return do_read(epollfd, transport->server_fd, transport);
}


int do_write(int epollfd, int sockfd, struct Transport *transport) {
    int nwrite;
    nwrite = write(sockfd, transport, BUF_SIZE);
    if (nwrite == -1) {
        perror("Write error:");
        delete_event(epollfd, sockfd, transport, EPOLLOUT);
    }
    else if (nwrite == 0) {
        fprintf(stderr, "Server closed.\n");
        delete_event(epollfd, sockfd, transport, EPOLLOUT);
    }else {
        modify_event(epollfd, sockfd, transport, EPOLLIN);
    }
    return nwrite;
}

int client_write(int epollfd, struct Transport *transport) {
    return do_write(epollfd, transport->client_fd, transport);
}
int server_write(int epollfd, struct Transport *transport) {
    return do_write(epollfd, transport->server_fd, transport);
}

//void handle_events(int epollfd, struct epoll_event events[], int num, char *buf) {
//    for (int i = 0; i < num; i++) {
//        if (events[i].events & EPOLLIN)
//            do_read(epollfd, events[i].data.fd, buf);
//        else if (events[i].events & EPOLLOUT)
//            do_write(epollfd, events[i].data.fd, buf);
//        else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) {
//            printf("epoll err %s, event fd: %d\n", strerror(errno), events[i].data.fd);
//            continue;
//        }
//    }
//}

