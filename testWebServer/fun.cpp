#include "fun.h"

void initialize_struct(int max, sockInfo* sockinfos){
    for(int i = 0; i < max; i++){
        bzero(&sockinfos[i], sizeof(sockinfos[i]));
        sockinfos[i].fd = -1;
        sockinfos[i].tid = -1;
    }
}

// output error message
void error_handling(const char* message){
    fputs(message, stderr);
    fputc('\0', stderr);
    exit(1);
}

// set file discription is noblocking
void setNoBlocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
}
