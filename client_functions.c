/*
Data communication Spring 2014
Lab 3 - Reliable Transportation Protocol
Students: sdn08003
          lja08001
*/

#include "protocol_std.h"
#include "client.h"

void prepareHostAddr(struct sockaddr_in* servAddr, char* hostName, int port)
{
    /*This function translates a host address string to a usable sockaddr_in struct*/
    printf("Preparing Host Address... ");
    struct hostent* hostInfo;
    memset((char*)servAddr,0,sizeof(*servAddr));
    servAddr->sin_family = AF_INET;
    servAddr->sin_port = htons(port);
    hostInfo = gethostbyname(hostName);
    if(hostInfo == NULL)
    {
        perror("prepareHostAddr - Unknown host\n");
        exit(EXIT_FAILURE);
    }
    servAddr->sin_addr = *(struct in_addr *)hostInfo->h_addr;
    printf("Host Address prepared!\n");
}

int clientSlidingWindow(int sfd, struct sockaddr_in* servAddr, int errorChance)
{
    /*
     *This function handles the sliding window functionality. It uses a
     *thread for user input and data sending. this function itself handles
     *received acks*/

    struct timeval shortTimeout;
    int numOfShortTimeouts = 0;
    pthread_t inputThread; // input thread handle

    struct windowStruct window; //window struct keeps track of the window
    window.sfd = sfd;
    window.servAddr = *servAddr;
    window.startSeq = 0;
    window.endSeq = 0;
    window.errorChance = errorChance;

    //folloing for loop sets all pointers in window.frameseq to NULL
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

    for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)//long timeout loop
    {
        resetShortTimeout(&shortTimeout);
        if(waitForFrame(sfd, &shortTimeout) == 1) //something to read
        {
            receivedFrame = receiveFrame(sfd,servAddr);

            if(receivedFrame->flags == ACK)
            {
                for(i=window.startSeq; i!=window.endSeq; i=(i+1)%MAXSEQ)//this loop steps through the modular sequence of sent frames
                {
                    if(receivedFrame->seq == i) //an ack in the window has been received, move startseq forward to that frame
                    {
                        printf("ACK received! Seq: %d\n",receivedFrame->seq);
                        window.startSeq = (receivedFrame->seq+1)%MAXSEQ;
                        free(receivedFrame);
                        numOfShortTimeouts = 0;//reset long timeout when a usable ack is received
                        break;
                    }
                }
                if(i==window.endSeq) //received frame is outside window, throw it away
                {
                    printf("Frame outside window received! Seq: %d \n",receivedFrame->seq);
                    free(receivedFrame);
                }
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
    // long timeout, cancel thread and return
    pthread_cancel(inputThread);
    return -1;
}

void* inputThreadFunction(void *arg)
{
    /*This function is the input thread. it takes input from the user and sends the data.
      This thread will be canceled when a long time out occurs.*/
    struct windowStruct *window = (struct windowStruct *)arg; //the argument is the window struct
    char messageString[MAXMSG];
    char ch;
    while(1)
    {
        //get message to send
        printf("Message: ");
        fflush(stdin);
        fgets(messageString,MAXMSG,stdin);
        messageString[MAXMSG-2] = '\n';
        messageString[MAXMSG-1] = '\0';

        //flushes stdin from overflow chars
        if(strlen(messageString)+1 == MAXMSG)
            while((ch=getchar())!='\n' && ch!=EOF);

        printf("Number of chars to send: %d\n",(int)strlen(messageString)+1);

        if(strncmp(messageString,"FIN\n",MAXMSG)==0) //if the word FIN is input the teardown process starts
        {
            return NULL;
        }
        else
        {
            //sending an info frame to the server. this contains the number of chars in the message
            printf("Sending INF. seq:%d\n",window->endSeq);
            if(window->frameSeq[window->endSeq] != NULL)
                free(window->frameSeq[window->endSeq]); //frees the next frame in sequence if it is not NULL
            window->frameSeq[window->endSeq] = newFrame(INF,window->endSeq,strlen(messageString)+1); //new INF frame
            sendFrame(window->sfd, window->frameSeq[window->endSeq], window->servAddr, window->errorChance); //send INF frame
            window->endSeq=(window->endSeq+1)%MAXSEQ; //step endseq forward

            int i;
            for(i=0;i<strlen(messageString)+1;i++) //loops through the message sending each char
            {
                //while the window is full, + and - WINDSIZE because of modularity
                while(window->endSeq-window->startSeq == WINDSIZE || window->endSeq-window->startSeq == -WINDSIZE)
                {
                    usleep(200000);
                    //if the window is still full after a short wait
                    if(window->endSeq-window->startSeq == WINDSIZE || window->endSeq-window->startSeq == -WINDSIZE)
                        resendFrames(window);
                }
                printf("Sending frame, seq:%d ... ",window->endSeq);

                if(window->frameSeq[window->endSeq] != NULL)
                    free(window->frameSeq[window->endSeq]); //frees the next frame in sequence if not NULL

                window->frameSeq[window->endSeq] = newFrame(ACK,window->endSeq,messageString[i]);
                sendFrame(window->sfd, window->frameSeq[window->endSeq], window->servAddr, window->errorChance);
                window->endSeq=(window->endSeq+1)%MAXSEQ;
                printf("frame sent!\n");
            }
            //while the window is not empty after sending all the chars
            while(window->endSeq-window->startSeq != 0)
            {
                usleep(200000);
                //if it is still not empty after a short wait, resend all frames in the window
                if(window->endSeq-window->startSeq != 0)
                    resendFrames(window);
            }
        }
    }
}

void resendFrames(struct windowStruct *window)
{
    /*This function resends all frames currently in the window*/
    int i;
    for(i = window->startSeq; i!=window->endSeq; i=(i+1)%MAXSEQ)
    {
        printf("Resending frame, seq:%d\n",i);
        sendFrame(window->sfd, window->frameSeq[i], window->servAddr, window->errorChance);
    }
}


