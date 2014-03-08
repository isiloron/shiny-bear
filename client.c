#include "protocol_std.h"
#include "client.h"


int main(int argc, char *argv[]){
    int sock_fd;
    struct sockaddr_in sockaddr;
    //rtp* frame;
    //struct Buffer* buffer;

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
            prepareSocket(&sock_fd, &sockaddr);
            //buffer = newBuffer();
            //int bytes_sent = sendto(sock_fd,);
            state = SYN_SENT;
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

