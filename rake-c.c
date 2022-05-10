#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "lib.h"




struct RAKEFILE {
    char port[10];
    char hosts[MAX_HOSTS_NUM][20];
    char *actions[10];
    int host_num;
    int action_num;
};

extern char **strsplit(const char *line, int *nwords);

extern void free_words(char **words);

int get_line(char **str, int *buff_size, FILE *fp) {
    if (NULL == *str) {
        *buff_size = 120;
        *str = malloc(120);
    } else {
        memset(*str, 0, strlen(*str));
    }
    int line_ch_size = 0;
    int ch;
    while (1) {
        if ((ch = fgetc(fp)) != EOF) {
            if (ch == '\n') {
                break;
            } else {
                if (line_ch_size == (*buff_size) - 1) {
                    *buff_size = (*buff_size) + 120;
                    *str = realloc(*str, *buff_size);
                }
                sprintf(*str, "%s%c", *str, ch);
                line_ch_size++;
            }

        } else {
            break;
        }
    }
    return line_ch_size;
}

void get_line_free(char **str) {
    if (NULL != *str) {
        free(*str);
    }
}

bool starts_with(const char *str, const char *prefix) {
    if (!str || !prefix)
        return 0;
    if (strncmp(str, prefix, strlen(prefix)) == 0)
        return 1;
    return 0;
}

bool end_with(const char *str, const char *prefix) {
    if (!str || !prefix)
        return 0;
    if (strncmp(str, prefix, strlen(str) - strlen(prefix)) == 0)
        return 1;
    return 0;
}

bool is_action(const char *str) {
    if (!str)
        return 0;
    if (strncmp(str, "    ", 4) == 0 || strncmp(str, "\t", 1) == 0)
        return 1;
    return 0;
}

bool is_requires(const char *str) {
    if (!str)
        return 0;
    if (strncmp(str, "        ", 8) == 0 || strncmp(str, "\t\t", 2) == 0)
        return 1;
    return 0;
}

void readRakefile(struct RAKEFILE *rakefile) {
    char dir[255];
    getcwd(dir, 255);
    strcat(dir, "/Rakefile");
    printf("The rakefile path is %s\n", dir);
    FILE *fp = fopen(dir, "r");
    if (fp == NULL) {
        printf("Couldn't open rakefile for reading\n");
        return;
    }
    char *str_line = NULL;
    int buff_size;
    int line_ch_size;

    bool in_actionset = false;
    int actionset_num = 0;
    int action_idx = 0;


    while ((line_ch_size = get_line(&str_line, &buff_size, fp)) != 0) {
        // comment or blank line
        if (str_line[0] == '#' || str_line[0] == '\r') {
            continue;
        } else if (starts_with(str_line, "actionset") == 1) {
            //获取actionset num
            //清空action_idx
            in_actionset = true;
        } else if (is_requires(str_line)) {

        } else if (is_action(str_line)) {
            char *str_begin = str_line;
            str_begin += ACTION_PREFIX_LENGTH;
            int str_size = line_ch_size - ACTION_PREFIX_LENGTH;
            rakefile->actions[rakefile->action_num] = malloc(str_size);
            strcpy(rakefile->actions[rakefile->action_num], str_begin);
//                FILE * file;
//                char buffer[80];
//                file=popen(rakefile->actions[rakefile->action_num], "r");
//                if(file==NULL) {
//                    printf("Action invoke failed: %s\n", strerror(errno));
//                    return;
//                }
//                fgets(buffer, sizeof(buffer), file);
//                printf("%s",buffer);
//                pclose(file);
            rakefile->action_num++;
        } else {
            int nwords;
            char **words = strsplit(str_line, &nwords);
            // variables
            if (nwords > 2 && strcmp(words[1], "=") == 0) {
                if (strcmp(words[0], "HOSTS") == 0) {
                    for (int w = 2; w < nwords; ++w) {
                        strcpy(rakefile->hosts[w - 2], words[w]);
                        rakefile->host_num++;
                    }
                } else if (strcmp(words[0], "PORT") == 0) {
                    strcpy(rakefile->port, words[2]);
                }
            }
            free_words(words);
        }
    }

    get_line_free(&str_line);
    //TODO 关闭文件报错
//    fclose(fp);
}



int client(char *ip, int port) {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket");
        exit(errno);
    }
    struct sockaddr_in servaddr;

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &servaddr.sin_addr);
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        printf("Connect %s:%d failed: %s\n", ip, port, strerror(errno));
        exit(errno);
    } else {
        printf("Connected %s:%d\n", ip, port);
    }
    return sockfd;
}


int main() {
    struct RAKEFILE rakefile;

    memset(&rakefile, 0, sizeof(rakefile));

    readRakefile(&rakefile);

    struct epoll_event events[MAX_EPOLL_EVENTS];
    bzero(events, sizeof(events));

    int epollfd = epoll_create(EPOLL_SIZE);

    int ready_cnt;
    int sock_fds[rakefile.host_num];

    //connect client_fd server
    for (int i = 0; i < rakefile.host_num; i++) {
        int port;
        char *idx;
        idx = strstr(rakefile.hosts[i], ":");
        if (idx != NULL) {
            port = atoi(idx + 1);
            *idx = '\0';
        } else {
            port = htons(atoi(rakefile.port));
        }
        sock_fds[i] = client(rakefile.hosts[i], port);
        struct Transport transport;
        transport.client_fd = sock_fds[i];
        client_add_event(epollfd, &transport, EPOLLOUT);
    }

    //query cost server_fd all host
    for (int i = 0; i < rakefile.action_num; i++) {

        ready_cnt = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, TIMEOUT);
        for (int j = 0; j < ready_cnt; j++) {
            if (events[j].events & EPOLLOUT) {
                struct Transport *transport = (struct Transport *) events[j].data.ptr;
                transport->method = QUERY;
                transport->server_fd = -1;
                strcpy(transport->data, rakefile.actions[i]);
                client_write(epollfd, transport);
            }
            //wait for data
            else if (events[i].events & EPOLLIN){
                struct Transport *transport = events[i].data.ptr;
                client_read(epollfd, transport);
//            memcpy(&transport, buf, BUF_SIZE);
                printf("%d\n", transport->method);
                printf("%d\n", transport->client_fd);
                printf("%d\n", transport->server_fd);
                printf("%s\n", transport->data);
            }
            else if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP) {
                printf("epoll err %s, event fd: %d\n", strerror(errno), events[i].data.fd);
            }
        }
    }
    //send action

    //wait response

    close(epollfd);
    return 0;
}
