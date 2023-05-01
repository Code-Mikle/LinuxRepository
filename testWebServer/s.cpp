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
#include <sys/select.h>
#include "fun.h"
#include "pthread_pool.h"

// struct sockInfo sockinfos[128];

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

    // create an fd_set, which stores the file descriptor to be detected.
    fd_set rdset, tmp;
    FD_ZERO(&rdset);
    FD_SET(servfd, &rdset);
    int maxfd = servfd; 

    // create thread_pool
    threadpool<int>*pool = NULL;
    try{
        pool = new threadpool<int>;
    }catch(...){
        return 1;
    }

    while(1){
        tmp = rdset;

        int ret = select(maxfd + 1, &tmp, NULL, NULL, NULL);
        if(ret == -1){
            error_handling("select() error!");
        }else if(ret == 0){
            continue;
        }else if(ret > 0){
            if(FD_ISSET(servfd, &tmp)){
                struct sockaddr_in clnt_adr;
                socklen_t clnt_adr_size = sizeof(clnt_adr);
                clntfd = accept(servfd, (struct sockaddr*)&clnt_adr, &clnt_adr_size);
                if(clntfd == 1){
                    error_handling("accept() error!");
                }
                // add the new descriptor into set
                FD_SET(clntfd, &rdset);

                // update the value of "maxfd"
                maxfd = maxfd > clntfd ? maxfd : clntfd;
            }

            for(int i = servfd + 1; i <= maxfd; i++){
                if(FD_ISSET(i, &tmp)){
                    pool->append(&i);
                }
            }
        }
    }

    // // initialize struct
    // int max = sizeof(sockinfos) / sizeof(sockinfos[0]);
    // initialize_struct(max, sockinfos);

    // while(1){
    //     struct sockaddr_in clnt_adr;
    //     socklen_t clnt_adr_size = sizeof(clnt_adr);

    //     // accept connection
    //     clntfd = accept(servfd, (struct sockaddr*)&clnt_adr, &clnt_adr_size);
    //     if(clntfd == -1){
    //         error_handling("accept() error!");
    //     }

    //     struct sockInfo* pinfo;
    //     for(int i = 0; i < max; i++){
    //         if(sockinfos[i].fd == -1){
    //             pinfo = &sockinfos[i];
    //             break;
    //         }

    //         if(i == max - 1){
    //             i = 0;
    //         }
    //     }

    //     pinfo->fd = clntfd;
    //     memcpy(&pinfo->addr, &clnt_adr, clnt_adr_size);

    //     // append into threadpool
    //     pool->append(pinfo);
    // }

    close(servfd);
    return 0;
}