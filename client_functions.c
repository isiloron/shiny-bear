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




int clientSlidingWindow(int sfd, struct sockaddr_in* servAddr, int errorChance)
{
    struct timeval shortTimeout;
    int numOfShortTimeouts = 0;
    pthread_t inputThread;

    struct windowStruct window;
    window.sfd = sfd;
    window.servAddr = *servAddr;
    window.startSeq = 0;
    window.endSeq = 0;
    window.errorChance = errorChance;
    int i;
    for(i=0;i<MAXSEQ;i++)
        window.frameSeq[i]=NULL;

    printf("Creating input thread!\n");
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
            receivedFrame = receiveFrame(sfd,servAddr);

            if(receivedFrame->flags == ACK)// the sequence is less than the last seq sent+1
            {
                for(i=window.startSeq; i!=window.endSeq; i=(i+1)%MAXSEQ)
                {
                    if(receivedFrame->seq == i)
                    {
                        printf("ACK received! Seq: %d\n",receivedFrame->seq);
                        window.startSeq = (receivedFrame->seq+1)%MAXSEQ;
                        free(receivedFrame);
                        break;
                    }
                }
                if(i==window.endSeq)
                {
                    printf("Frame outside window received! Seq: %d \n",receivedFrame->seq);
                    free(receivedFrame);
                }
                numOfShortTimeouts = 0;
            }
            else /*received unexpected packet*/
            {
                printf("Unexpected packet received! \n");
                free(receivedFrame);
            }
        }
        else if(pthread_tryjoin_np(inputThread,NULL)==0) //input thread has closed i.e. it is time to tear down
        {
            return 0; //application close
        }
    }
    /// long timeout, stäng av lästråden och returnera -1
    pthread_cancel(inputThread);
    return -1;
}

void* inputThreadFunction(void *arg)
{
    srand(time(0));
    struct windowStruct *window = (struct windowStruct *)arg;
    char messageString[MAXMSG];
    char ch;
    while(1)
    {
        printf("Message: ");
        fflush(stdin);
        fgets(messageString,MAXMSG,stdin);
        messageString[MAXMSG-2] = '\n';
        messageString[MAXMSG-1] = '\0';
        if(strlen(messageString)+1 == MAXMSG)
            while(ch=getchar()!='\n' && ch!=EOF);
        printf("Number of chars to send: %d\n",(int)strlen(messageString)+1);
        if(strncmp(messageString,"FIN\n",MAXMSG)==0)
        {
            return NULL;
        }
        else
        {
            ///Först en info
            printf("Sending INF. seq:%d\n",window->endSeq);
            if(window->frameSeq[window->endSeq] != NULL)
                free(window->frameSeq[window->endSeq]); //frees the next frame in sequence
            window->frameSeq[window->endSeq] = newFrame(INF,window->endSeq,strlen(messageString)+1);
            sendFrame(window->sfd, window->frameSeq[window->endSeq], window->servAddr, window->errorChance);
            window->endSeq=(window->endSeq+1)%MAXSEQ;

            ///Sedan alla andra frames

            int i;
            for(i=0;i<strlen(messageString)+1;i++)
            {
                while(window->endSeq-window->startSeq == WINDSIZE || window->endSeq-window->startSeq == -WINDSIZE)
                {
                    usleep(200000);
                    if(window->endSeq-window->startSeq == WINDSIZE || window->endSeq-window->startSeq == -WINDSIZE)
                        resendFrames(window);
                }
                printf("Sending frame, seq:%d ... ",window->endSeq);

                if(window->frameSeq[window->endSeq] != NULL)
                    free(window->frameSeq[window->endSeq]); //frees the next frame in sequence

                window->frameSeq[window->endSeq] = newFrame(ACK,window->endSeq,messageString[i]);
                sendFrame(window->sfd, window->frameSeq[window->endSeq], window->servAddr, window->errorChance);
                window->endSeq=(window->endSeq+1)%MAXSEQ;
                printf("frame sent!\n");
            }
            while(window->endSeq-window->startSeq != 0)
            {
                usleep(200000);
                if(window->endSeq-window->startSeq != 0)
                    resendFrames(window);
            }
        }
    }
}

void resendFrames(struct windowStruct *window)
{
    int i;
    for(i = window->startSeq; i!=window->endSeq; i=(i+1)%MAXSEQ)
    {
        printf("Resending frame, seq:%d\n",i);
        sendFrame(window->sfd, window->frameSeq[i], window->servAddr, window->errorChance);
    }
}


