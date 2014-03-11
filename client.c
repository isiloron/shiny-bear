#include "protocol_std.h"
#include "client.h"

//STATES
#define CLOSED 0
#define SYN_SENT 1
#define PRECAUTION 2
#define ESTABLISHED 3
#define INIT_TEARDOWN 4

int main(int argc, char *argv[])
{
    int sfd;
    struct sockaddr_in myAddr, servAddr;
    rtp* frameToSend;
    rtp* receivedFrame;
    //int winStartSeq = 0;
    //int winEndSeq = 0;
    //int bytesSent = 0;
    char hostName[hostNameLength];
    struct timeval shortTimeout;
    int numOfShortTimeouts = 0;

    if(argv[1] == NULL)
    {
        perror("Usage: client [host name]");
        exit(EXIT_FAILURE);
    }
    else
    {
        strncpy(hostName, argv[1], hostNameLength);
        hostName[hostNameLength - 1] = '\0';
    }

    prepareHostAddr(&servAddr, hostName, PORT);

    int state = CLOSED;
    while(1)
    {
        switch(state)
        {
        case CLOSED:
            prepareSocket(&sfd, &myAddr);
            frameToSend = newFrame(SYN,0,0,0);
            sendFrame(sfd, frameToSend, servAddr);
            printf("Syn sent!\n");
            free(frameToSend);
            state = SYN_SENT;
            break;
        case SYN_SENT:
            for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
            {
                resetShortTimeout(&shortTimeout);
                if(waitForFrame(sfd, &shortTimeout) == 0) // short timeout
                {
                    frameToSend = newFrame(SYN,0,0,0);
                    sendFrame(sfd, frameToSend, servAddr);
                    free(frameToSend);
                    printf("Resent SYN.\n");
                }
                else //frame to read
                {
                    receivedFrame = receiveFrame(sfd, &servAddr);
                    if(receivedFrame->flags == SYN+ACK)
                    {
                        printf("SYN+ACK received!\n");
                        frameToSend = newFrame(ACK,0,0,0);
                        sendFrame(sfd, frameToSend, servAddr);
                        free(frameToSend);
                        free(receivedFrame);
                        state = PRECAUTION;
                        break;
                    }
                    free(receivedFrame);
                }
            }
            if(state==PRECAUTION)
            {
                break;
            }
            printf("Long timeout! State CLOSED.\n");
            close(sfd);
            state = CLOSED;
            break;
        case PRECAUTION:
            for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
            {
                resetShortTimeout(&shortTimeout);
                if(waitForFrame(sfd,&shortTimeout) == 0)
                {
                    state = ESTABLISHED;
                    break;
                }
                else
                {
                    receivedFrame = receiveFrame(sfd, &servAddr);
                    if(receivedFrame->flags == SYN+ACK)
                    {
                        printf("SYN+ACK received!\n");
                        frameToSend = newFrame(ACK,0,0,0);
                        sendFrame(sfd, frameToSend, servAddr);
                        free(frameToSend);
                        printf("Resent ACK.\n");
                    }
                    free(receivedFrame);
                }
            }
            if(state==ESTABLISHED)
            {
                break;
            }
            printf("Long timeout! State CLOSED.\n");
            close(sfd);
            state = CLOSED;
            break;
        case ESTABLISHED:
            printf("ESTBLISHED STATE REACHED!\n");
            //state = clientSlidingWindow(sfd, servAddr);
            /*char message[50];
            for(;;)
            {
                fgets(message,50,stdin);
                message[50-1] = '\0';
                if(strncmp(message,"quit\n",50)==0)
                {*/
            frameToSend = newFrame(FIN,0,0,0);
            sendFrame(sfd, frameToSend, servAddr);
            free(frameToSend);
            state = INIT_TEARDOWN;
            /*break;
                            }
                        }*/
            break;
        case FIN_SENT:
            for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
            {
                resetShortTimeout(shortTimeout);

                if(waitForFrame(fd, shortTimeout) == 0) // short timeout, Resend FIN
                {
                    frameToSend = newFrame(FIN, 0, 0, 0);//recreate a FIN frame
                    sendFrame(fd, frameToSend, *sourceAddr); /*resend FIN*/
                    printf("FIN resent \n");
                    free(frameToSend);
                }
                else //frame to read
                {
                    receivedFrame = receiveFrame(fd, sourceAddr);

                    if(receivedFrame->flags == ACK)/*received expected packet ACK*/
                    {
                        printf("ACK received!\n");
                        state = AWAIT_FIN;
                        break;
                    }
                    else if(receivedFrame->flags == FIN) /*received expected packet FIN (simultaneous close)*/
                    {
                        printf("FIN received \n");
                        frameToSend = newFrame(ACK, 0, 0, 0);//create a ACK frame
                        sendFrame(fd, frameToSend, *sourceAddr);
                        printf("ACK SENT \n");
                        free(receivedFrame);
                        state = SIMULTANEOUS_CLOSE;
                        break;
                    }
                    else /*received unexpected packet*/
                    {
                        printf("Unexpected packet received! \n");
                        free(receivedFrame);
                        break;
                    }
                }
            }/*End of for-loop*/
            if( (state==SIMULTANEOUS_CLOSE) || (state==AWAIT_FIN) )
            {
                break;
            }

            printf("Long timeout! Closing socket.\n");
            state = CLOSED;
            break;
        case SIMULTANEOUS_CLOSE:
            for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
            {
                resetShortTimeout(shortTimeout);

                if(waitForFrame(fd, shortTimeout) == 0) // short timeout, Resend ACK
                {
                    frameToSend = newFrame(ACK, 0, 0, 0);//recreate a ACK frame
                    sendFrame(fd, frameToSend, *sourceAddr); /*resend ACK*/
                    printf("ACK resent \n");
                    free(frameToSend);
                }
                else //frame to read
                {
                    receivedFrame = receiveFrame(fd, sourceAddr);

                    if(receivedFrame->flags == ACK)/*received expected packet ACK*/
                    {
                        printf("ACK received!\n");
                        state = SHORT_WAIT;
                        free(receivedFrame);
                        break;
                    }
                    else /*received unexpected packet*/
                    {
                        printf("Unexpected packet received! \n");
                        free(receivedFrame);
                        break;
                    }
                }
            }/*End of for-loop*/
            if( (state==SIMULTANEOUS_CLOSE) || (state==AWAIT_FIN) )
            {
                break;
            }

            printf("Long timeout! Closing socket.\n");
            state = CLOSED;
            return state;
        case AWAIT_FIN:
            for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
            {
                resetShortTimeout(shortTimeout);

                if(waitForFrame(fd, shortTimeout) == 0) {} // short timeout, do nothing
                else //frame to read
                {
                    receivedFrame = receiveFrame(fd, sourceAddr);

                    if(receivedFrame->flags == FIN)/*received expected packet FIN*/
                    {
                        printf("FIN received!\n");
                        state = SHORT_WAIT;
                        free(receivedFrame);
                        break;
                    }
                    else /*received unexpected packet*/
                    {
                        printf("Unexpected packet received! \n");
                        free(receivedFrame);
                        break;
                    }
                }
            }/*End of for-loop*/
            if(state==SHORT_WAIT)
            {
                break;
            }

            printf("Long timeout! Closing socket.\n");
            state = CLOSED;
            return state;
        case SHORT_WAIT:
            resetShortTimeout(shortTimeout);

            if(waitForFrame(fd, shortTimeout) == 0)// short timeout, socket closed successfully
            {
                printf("Socket closed successfully \n");
                state = CLOSED;
                return state;
            }
        default:
            perror("Undefined state.");
            exit(EXIT_FAILURE);
            break;
        }
    }
    return 0;
}

