#include "protocol_std.h"
#include "client.h"


void prepareHostAddr(struct sockaddr_in* servAddr, char* hostName, int port)
{
    printf("Preparing Host Address... ");
    struct hostent* hostInfo;
    memset((char*)servAddr,0,sizeof(*servAddr));
    servAddr->sin_family = AF_INET;
    servAddr->sin_port = htons(port);
    hostInfo = gethostbyname(hostName);
    if(hostInfo == NULL)
    {
        fprintf(stderr, "prepareHostAddr - Unknown host %s\n",hostName);
        exit(EXIT_FAILURE);
    }
    servAddr->sin_addr = *(struct in_addr *)hostInfo->h_addr;
    printf("Host Address prepared!\n");
}

int clientSlidingWindow(int sfd, struct sockaddr_in* servAddr)
{
    rtp* frameToSend;
    rtp* receivedFrame;
    char messageString[MAXMSG];
    struct Buffer* buffer;

    while(1)
    {
        fflush(stdin);
        fgets(messageString,MAXMSG,stdin);
        if(strncmp(messageString,"FIN\n",MAXMSG)==0)
        {
            return 0;
        }
        else
        {
            printf("Sending 'A'\n");
            frameToSend = newFrame(ACK,0,0,'A');
            buffer = newBuffer();
            serializeFrame(frameToSend,buffer);
            buffer->next = 0;
            receivedFrame = newFrame(0,0,0,0);
            deserializeFrame(receivedFrame,buffer);
            printf("%c\n",frameToSend->data);
            printf("%c\n",receivedFrame->data);
            sendFrame(sfd, frameToSend, *servAddr);
            free(frameToSend);
        }
    }
}
