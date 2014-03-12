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
    //rtp* receivedFrame;
    char messageString[MAXMSG];
    /*struct Buffer* buffer;
    struct timeval shortTimeout;
    int numOfShortTimeouts = 0;

    struct WindowStruct
    {
        rtp* frameSeq[MAXSEQ];
        int startSeq;
        int endSeq;
    };

    struct WindowStruct window;
    window.startSeq = 0;
    window.endSeq = 0;*/

    //tråd som tar in ett meddelande från användaren och skickar iväg frames

    //select loop med timeout som läser ackar.

    int seq = 0;
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
            printf("Sending INF.\n");
            frameToSend = newFrame(INF,seq,0,strlen(messageString));
            sendFrame(sfd, frameToSend, *servAddr);
            seq=(seq+1)%MAXSEQ;
            free(frameToSend);
            int i;
            for(i=0;i<strlen(messageString);i++)
            {
                frameToSend = newFrame(ACK,seq,0,messageString[i]);
                sendFrame(sfd, frameToSend, *servAddr);
                seq=(seq+1)%MAXSEQ;
                free(frameToSend);
            }
            frameToSend = newFrame(ACK,seq,0,'\0');
            sendFrame(sfd, frameToSend, *servAddr);
            seq=(seq+1)%MAXSEQ;
            free(frameToSend);

            /*
            printf("Sending 'A'\n");
            frameToSend = newFrame(ACK,10,-7,messageString[0]);
            buffer = newBuffer();
            serializeFrame(frameToSend,buffer);
            buffer->next = 0;
            receivedFrame = newFrame(0,0,0,0);
            deserializeFrame(receivedFrame,buffer);
            printf("%d %d %d %c\n",frameToSend->flags,frameToSend->seq,frameToSend->crc,frameToSend->data);
            printf("%d %d %d %c\n",receivedFrame->flags,receivedFrame->seq,receivedFrame->crc,receivedFrame->data);
            sendFrame(sfd, frameToSend, *servAddr);
            free(frameToSend);
            */
        }
    }
}
