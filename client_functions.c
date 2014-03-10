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

/*
void* getInput(void* arg)
{
    fgets(messageString,MAXMSG,stdin);
    messageString[MAXMSG-1] = '\0';
}

int clientSlidingWindow(int sfd, struct sockaddr_in servAddr)
{
    //rtp* frameToSend;
    //rtp* receivedFrame;
    char messageString[MAXMSG];
    pthread_t readThreadId;

    if(pthread_create(&readThreadId,NULL,&readMessageFromServer, messageString)!=0)
        error("Could not create reading thread!\n");



}
*/
