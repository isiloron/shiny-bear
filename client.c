#include "protocol_std.h"
#include "client.h"

//STATES
#define CLOSED 0
#define SYN_SENT 1
#define PRECAUTION 2
#define ESTABLISHED 3
#define FIN_SENT 4
#define AWAIT_FIN 5
#define SHORT_WAIT 6

int main(int argc, char *argv[])
{
    srand(time(0));

    int sfd;
    struct sockaddr_in myAddr;
    struct sockaddr_in servAddr;
    rtp* frameToSend;
    rtp* receivedFrame;
    char hostName[hostNameLength];
    struct timeval shortTimeout;
    int numOfShortTimeouts = 0;
    int errorChance = 0;

    if(argv[1] == NULL)
    {
        perror("Usage: client [host name] [error percentage]");
        exit(EXIT_FAILURE);
    }
    else
    {
        strncpy(hostName, argv[1], hostNameLength);
        hostName[hostNameLength - 1] = '\0';
    }

    if(argv[2] == NULL)
    {
        perror("Usage: client [host name] [error percentage]");
        exit(EXIT_FAILURE);
    }
    else
    {
        errorChance = atoi(argv[2]);
        if(errorChance<0)
            errorChance = 0;
        else if (errorChance>100)
            errorChance = 100;
    }

    prepareHostAddr(&servAddr, hostName, PORT);

    int state = CLOSED;
    while(1)
    {
        switch(state)
        {
            case CLOSED:
                printf("Entered CLOSED state!\n");
                prepareSocket(&sfd, &myAddr);
                frameToSend = newFrame(SYN,0,0);
                sendFrame(sfd, frameToSend, servAddr, errorChance);
                printf("Syn sent!\n");
                free(frameToSend);
                state = SYN_SENT;
                break;

            case SYN_SENT:
                printf("Entered SYN_SENT state!\n");
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    resetShortTimeout(&shortTimeout);
                    if(waitForFrame(sfd, &shortTimeout) == 0) // short timeout
                    {
                        frameToSend = newFrame(SYN,0,0);
                        sendFrame(sfd, frameToSend, servAddr, errorChance);
                        free(frameToSend);
                        printf("Resent SYN.\n");
                    }
                    else //frame to read
                    {
                        receivedFrame = receiveFrame(sfd, &servAddr);
                        if(receivedFrame->flags == SYN+ACK)
                        {
                            printf("SYN+ACK received!\n");
                            printf("Sending ACK!\n");
                            frameToSend = newFrame(ACK,0,0);
                            sendFrame(sfd, frameToSend, servAddr, errorChance);
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
                printf("Entered PRECAUTION state!\n");
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    shortTimeout.tv_sec = 1;
                    shortTimeout.tv_usec = 0;
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
                            frameToSend = newFrame(ACK,0,0);
                            sendFrame(sfd, frameToSend, servAddr, errorChance);
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
                printf("Entered ESTABLISHED state!\n");
                if(clientSlidingWindow(sfd, &servAddr, errorChance) == 0)
                {
                    frameToSend = newFrame(FIN,0,0);
                    sendFrame(sfd, frameToSend, servAddr, errorChance);
                    free(frameToSend);
                    state = FIN_SENT;
                }
                else
                {
                    printf("Long timeout! State CLOSED.\n");
                    close(sfd);
                    state = CLOSED;
                }
                break;

            case FIN_SENT:
                printf("FIN sent!\n");
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    resetShortTimeout(&shortTimeout);

                    if(waitForFrame(sfd, &shortTimeout) == 0) // short timeout, Resend FIN
                    {
                        frameToSend = newFrame(FIN,0,0);//recreate a FIN frame
                        sendFrame(sfd, frameToSend, servAddr, errorChance); /*resend FIN*/
                        printf("FIN resent!\n");
                        free(frameToSend);
                    }
                    else //frame to read
                    {
                        receivedFrame = receiveFrame(sfd, &servAddr);

                        if(receivedFrame->flags == ACK)/*received expected packet ACK*/
                        {
                            printf("ACK received!\n");
                            state = AWAIT_FIN;
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
                if(state==AWAIT_FIN)
                {
                    break;
                }
                printf("Long timeout! Closing socket.\n");
                close(sfd);
                state = CLOSED;
                break;

            case AWAIT_FIN:
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    resetShortTimeout(&shortTimeout);

                    if(waitForFrame(sfd, &shortTimeout) == 0) {} // short timeout, do nothing
                    else //frame to read
                    {
                        receivedFrame = receiveFrame(sfd, &servAddr);

                        if(receivedFrame->flags == FIN)/*received expected packet FIN*/
                        {
                            printf("FIN received!\n");
                            frameToSend = newFrame(ACK,0,0);//recreate a FIN frame
                            sendFrame(sfd, frameToSend, servAddr, errorChance); /*resend FIN*/
                            printf("ACK sent!\n");
                            free(frameToSend);
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
                close(sfd);
                state = CLOSED;
                break;

            case SHORT_WAIT:
                usleep(200000);
                close(sfd);
                state = CLOSED;
                printf("Connection was torn down!\n");
                break;

            default:
                perror("Undefined state.");
                exit(EXIT_FAILURE);
                break;
        }
    }
    return 0;
}

