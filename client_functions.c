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
    int errorChance;
};

int clientSlidingWindow(int sfd, struct sockaddr_in* servAddr, int errorChance)
{
    struct timeval shortTimeout;
    int numOfShortTimeouts = 0;
    pthread_t inputThread;

    struct WindowStruct window;
    window.sfd = sfd;
    window.servAddr = *servAddr;
    window.startSeq = 0;
    window.endSeq = 0;
    window.errorChance = errorChance;

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
    pthread_cancel(inputThread);
    return 1;
}

void* inputThreadFunction(void *arg)
{
    struct WindowStruct *window = (struct WindowStruct *)arg;
    char messageString[MAXMSG];
    while(1)
    {
        fflush(stdin);
        fgets(messageString,MAXMSG,stdin);
        if(strncmp(messageString,"FIN\n",MAXMSG)==0)
        {
            return NULL;
        }
        else
        {
            ///Först en info
            printf("Sending INF.\n");
            free(window->frameSeq[window->endSeq]); //frees the next frame in sequence
            window->frameSeq[window->endSeq] = newFrame(INF,window->endSeq,strlen(messageString)+1);
            sendFrame(window->sfd, window->frameSeq[window->endSeq], window->servAddr, window->errorChance);
            window->endSeq=(window->endSeq+1)%MAXSEQ;

            ///Sedan alla andra frames

            int i;
            for(i=0;i<strlen(messageString)+1;i++)
            {
                while(window->endSeq-window->startSeq == WINDSIZE)
                {
                    usleep(200000);
                    ///resend all sent frames
                }
                free(window->frameSeq[window->endSeq]);
                window->frameSeq[window->endSeq] = newFrame(ACK,window->endSeq,messageString[i]);
                sendFrame(window->sfd, window->frameSeq[window->endSeq], window->servAddr, window->errorChance);
                window->endSeq=(window->endSeq+1)%MAXSEQ;
            }
            while(window->endSeq-window->startSeq != 0)
            {
                usleep(200000);
                ///resend all sent frames
            }
        }
    }
}
