#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "fun.h"

void error_handling(const char* message){
    fputs(message, stderr);
    fputc('\0', stderr);
    exit(1);
}

int main(int argc, char* argv[])
{
    if(argc != 3){
        printf("Usage %s <IP> <port>\n", argv[0]);
    }

    int servfd = socket(PF_INET, SOCK_STREAM, 0);
    if(servfd == -1){
        error_handling("socket() error!");
    }

    struct sockaddr_in serv_adr;
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &serv_adr.sin_addr.s_addr);
    if(connect(servfd, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1){
        error_handling("connect() error!");
    }

    char message[] = "Hello, I'm client. Tell me what time it is now?";
    char recv_message[30] = {0};
    int idx = 0;
    while(idx < 10){
        int ret = write(servfd, message, sizeof(message));

        sleep(1);

        ret = read(servfd, recv_message, sizeof(recv_message));
        printf("client recv message: %s\n", recv_message);
        bzero(recv_message, 0);

        idx++;
    }
    close(servfd);
    return 0;
}