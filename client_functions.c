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


struct WindowStruct
{
    int sfd;
    struct sockaddr_in servAddr;
    rtp* frameSeq[MAXSEQ];
    int startSeq;
    int endSeq; // is one greater than the last seq sent, equal to startSeq if no frames has been sent, endSeq-startSeq = number of frames sent
};

int clientSlidingWindow(int sfd, struct sockaddr_in* servAddr)
{
    char messageString[MAXMSG];
    struct Buffer* buffer;
    struct timeval shortTimeout;
    int numOfShortTimeouts = 0;
    pthread_t inputThread;

    struct WindowStruct window;
    window.sfd = sfd;
    window.servAddr = *servAddr;
    window.startSeq = 0;
    window.endSeq = 0;

    ///tråd som tar in ett meddelande från användaren och skickar iväg frames
    if(pthread_create(&inputThread, NULL, &inputThreadFunction, &window) != 0)
    {
        perror("Could not create thread!");
        exit(EXIT_FAILURE);
    }

    rtp* receivedFrame = NULL;

    for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
    {
        resetShortTimeout(&shortTimeout);
        if(waitForFrame(sfd, &shortTimeout) == 1) //something to read
        {
            ///läser ackar
            receivedFrame = receiveFrame(sfd,servAddr);

            if(receivedFrame->flags == ACK //it is an ACK
               && receivedFrame->seq >= window.startSeq // it is the expected ack or greater
               && receivedFrame->seq < window.endSeq)// the sequence is less than the last seq sent+1
            {
                printf("ACK received! Seq: %d\n",receivedFrame->seq);
                window.startSeq = receivedFrame->seq+1;
                free(receivedFrame);
            }
            else /*received unexpected packet*/
            {
                printf("Unexpected packet received! \n");
                free(receivedFrame);
            }
        }
        else if(pthread_tryjoin_np(inputThread,NULL)==0) //input thread has closed i.e. it is time to tear down
        {
            return 0;
        }

    }

    /// long timeout, stäng av lästråden och returnera 1


    /**
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

        }
    }*/
}

void* inputThreadFunction(void *arg)
{

    return 0;
}
