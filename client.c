#include "client.h"


int main(int argc, char *argv[]){
    int sock_fd;
    struct sockaddr_in sockaddr;

    if((sock_fd=socket(AF_INET, SOCK_DGRAM, 0))<0){
        perror("Could not create socket.");
        return 0;
    }

    memset((char*)&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(PORT);

    if (bind(sock_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
        perror("Bind failed");
        return 0;
    }

    //sendto(int sock_fd, const void *buffer, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);

    #define CLOSED 0
    #define SYN_SENT 1
    #define PRECAUTION 2
    #define ESTABLISHED 3
    #define INIT_TEARDOWN 4
    #define RESP_TEARDOWN 5

    int state = CLOSED;
    switch(state){
        case CLOSED:
            break;
        case SYN_SENT:
            break;
        case PRECAUTION:
            break;
        case ESTABLISHED:
            break;
        case INIT_TEARDOWN:
            break;
        case RESP_TEARDOWN:
            break;
        default:
            perror("Undefined state.");
            break;
    }
    return 0;
}

