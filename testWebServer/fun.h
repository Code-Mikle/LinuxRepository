#ifndef FUN_H
#define FUN_H

#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>

struct sockInfo{
    int fd;
    struct sockaddr_in addr;
    pthread_t tid;
};

void initialize_struct(int max, sockInfo* sockinfos);

// output error message
void error_handling(const char* message);

// set file discription is noblocking
void setNoBlocking(int fd);

#endif