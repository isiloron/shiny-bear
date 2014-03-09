#include "protocol_std.h"
#include "client.h"

int main(int argc, char *argv[]){
    int sfd;
    struct sockaddr_in myAddr, servAddr;
    rtp* sendFrame/*, recieveFrame*/;
    struct Buffer* buffer;
    //int winStartSeq = 0;
    //int winEndSeq = 0;
    //int bytesSent = 0;
    char hostName[hostNameLength];

    if(argv[1] == NULL){
        perror("Usage: client [host name]");
        exit(EXIT_FAILURE);
    }
    else {
        strncpy(hostName, argv[1], hostNameLength);
        hostName[hostNameLength - 1] = '\0';
    }

    prepareHostAddr(&servAddr, hostName, PORT);

    #define CLOSED 0
    #define SYN_SENT 1
    #define PRECAUTION 2
    #define ESTABLISHED 3
    #define INIT_TEARDOWN 4
    #define RESP_TEARDOWN 5

    int state = CLOSED;
    while(1){
        switch(state){
            case CLOSED:
                prepareSocket(&sfd, &myAddr);
                buffer = newBuffer();
                sendFrame = newFrame(SYN,0,0,0);
                serializeFrame(sendFrame, buffer);
                /*bytesSent =*/ sendto(sfd, buffer->data, sizeof(*buffer->data), 0, (struct sockaddr*)&servAddr, sizeof(servAddr));
                printf("Syn sent!\n");
                state = SYN_SENT;
                break;
            case SYN_SENT:
                while(1);
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
    }
    return 0;
}

