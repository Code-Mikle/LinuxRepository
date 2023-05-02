#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include "pthread_pool.h"
#include "http_conn.h"

// struct sockInfo sockinfos[128];
#define MAX_EVENT_NUMBER 50  // the maximum of events monitored
#define MAX_FD 65536 // maximum number of flie descriptor

extern void addfd(int epollfd, int fd, bool one_shot);
extern void removefd(int epollfd, int fd);

// 添加信号捕捉
void addsig(int sig, void( handler )(int)){
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}

int main(int argc, char* argv[])
{
    // determine whether the input parameters are legal
    if(argc != 2){
        printf("Usage %s <port>\n", argv[0]);
        return 1;
    }
    
    // servfd: listening file descriptor. clntfd: connection file descriptor.
    int servfd, clntfd;

    addsig( SIGPIPE, SIG_IGN );
    
    // create thread_pool
    threadpool<http_conn>*pool = NULL;
    try{
        pool = new threadpool<http_conn>;
    }catch(...){
        return 1;
    }

    // save all client information
    http_conn* users = new http_conn[MAX_FD];

    // create socket
    servfd = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_adr;
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = INADDR_ANY;
    serv_adr.sin_port = htons(atoi(argv[1]));

    // set port reuse
    int reuse = 1;
    setsockopt(servfd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    
    // bind local port
    int ret = bind(servfd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));

    // listening for connection requests from 'servfd', with a maximum of 5 connections allowed.
    listen(servfd, 8);
    
    // create epoll objects and event arrays
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    addfd(epollfd, servfd, false);
    http_conn::m_epollfd = epollfd;
    
    // int idx = 0;
    while(true){
        int event_cnt = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if((event_cnt < 0) && (errno != EINTR)){
            printf("epoll failure\n");
            break;
        }

        // printf("%dth event_cnt is: %d\n", idx++, event_cnt);  
        for(int i = 0; i < event_cnt; i++){
            int sockfd = events[i].data.fd;

            if(sockfd == servfd){
                struct sockaddr_in clnt_adr;
                socklen_t clnt_adr_size = sizeof(clnt_adr);
                clntfd = accept(servfd, (struct sockaddr*)&clnt_adr, &clnt_adr_size);
                
                if(clntfd < 0){
                    printf("errno is: %d\n", errno);
                    continue;
                }

                if(http_conn::m_user_count >= MAX_FD){
                    close(clntfd);
                    continue;
                }

                users[clntfd].init(clntfd, clnt_adr);
            }else if(events[i].events & (EPOLLERR | EPOLLRDHUP | EPOLLERR)){
                users[sockfd].close_conn();
            }else if(events[i].events & EPOLLIN){
                // printf("it's read event!\n");
                if(users[sockfd].read_data()){
                    pool->append(users + sockfd);
                }else{
                    users[sockfd].close_conn();
                }
            }else if(events[i].events & EPOLLOUT){
                if(!users[sockfd].write()){
                    users[sockfd].close_conn();
                }
            }
        }
    } 
    close(epollfd);
    close(servfd);
    delete []users;
    delete pool;
    return 0;
}