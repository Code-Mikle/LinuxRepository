#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "fun.h"
#include "pthread_pool.h"

// struct sockInfo sockinfos[128];
#define MAX_EVENT_NUMBER 50  // the maximum of events monitored

int main(int argc, char* argv[])
{
    // determine whether the input parameters are legal
    if(argc != 2){
        printf("Usage %s <port>\n", argv[0]);
    }
    
    // servfd: listening file descriptor. clntfd: connection file descriptor.
    int servfd, clntfd;
    
    // create socket
    servfd = socket(PF_INET, SOCK_STREAM, 0);
    if(servfd == -1){
        error_handling("socket() error!");
    }

    // set port reuse
    int reuse = 1;
    setsockopt(servfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    
    struct sockaddr_in serv_adr;
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = INADDR_ANY;
    serv_adr.sin_port = htons(atoi(argv[1]));
    // bind local port
    int ret = bind(servfd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
    if(ret == -1){
        error_handling("bind() error!");
    }

    // listening for connection requests from 'servfd', with a maximum of 5 connections allowed.
    if(listen(servfd, 8) == -1){
        error_handling("listen() error!");
    }

    // create thread_pool
    threadpool<int>*pool = NULL;
    try{
        pool = new threadpool<int>;
    }catch(...){
        return 1;
    }

    struct epoll_event event;
    int event_cnt = 0;
    // create epoll objects and event arrays
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);

    threadpool<int>::init_epollfd(epollfd);

    event.events = EPOLLIN;
    event.data.fd = servfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, servfd, &event);
    
    char buffer[100] = {0};
    int idx = 0;
    while(1){
        event_cnt = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if(event_cnt == -1){
            error_handling("epoll_wait() error!");
        }

        printf("%dth event_cnt is: %d\n", idx++, event_cnt);  
        for(int i = 0; i < event_cnt; i++){
            if(events[i].data.fd == servfd){
                struct sockaddr_in clnt_adr;
                socklen_t clnt_adr_size = sizeof(clnt_adr);
                clntfd = accept(servfd, (struct sockaddr*)&clnt_adr, &clnt_adr_size);
                event.events = EPOLLIN;
                event.data.fd = clntfd;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, clntfd, &event);
                printf("connected client: %d\n", clntfd);
            }else{
                pool->append(events[i].data.fd);
            }
                
                // int str_len = read(events[i].data.fd, buffer, sizeof(buffer));
                // if(str_len == 0){
                //     epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                //     close(events[i].data.fd);
                //     printf("closed client: %d\n", events[i].data.fd);
                // }else{
                //     write(events[i].data.fd, buffer, str_len);
                // }
            
        }
    } 
    close(epollfd);
    close(servfd);

    // while(1){
    //     tmp = rdset;

    //     int ret = select(maxfd + 1, &tmp, NULL, NULL, NULL);
    //     if(ret == -1){
    //         error_handling("select() error!");
    //     }else if(ret == 0){
    //         continue;
    //     }else if(ret > 0){
    //         if(FD_ISSET(servfd, &tmp)){
    //             struct sockaddr_in clnt_adr;
    //             socklen_t clnt_adr_size = sizeof(clnt_adr);
    //             clntfd = accept(servfd, (struct sockaddr*)&clnt_adr, &clnt_adr_size);
    //             if(clntfd == 1){
    //                 error_handling("accept() error!");
    //             }
    //             // add the new descriptor into set
    //             FD_SET(clntfd, &rdset);

    //             // update the value of "maxfd"
    //             maxfd = maxfd > clntfd ? maxfd : clntfd;
    //         }

    //         for(int i = servfd + 1; i <= maxfd; i++){
    //             if(FD_ISSET(i, &tmp)){
    //                 pool->append(&i);
    //             }
    //         }
    //     }
    // }

    return 0;
}