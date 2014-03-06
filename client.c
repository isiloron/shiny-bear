#include "client.h"


int main(int argc, char *argv[]){
    int sock_fd;

    if((sock_fd=socket(AF_INET, SOCK_DGRAM, 0))<0){
        perror("Could not create socket.");
        return 0;
    }


    int state;

    #define CLOSED 0
    #define SYN_SENT 1
    #define PRECAUTION 2
    #define ESTABLISHED 3
    #define INIT_TEARDOWN 4
    #define RESP_TEARDOWN 5

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
            error("Undefined state.")
            break;
    }
}

