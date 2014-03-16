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
    srand(time(0));//random seed
    int sfd; //socket file descriptor
    struct sockaddr_in myAddr; //local address
    struct sockaddr_in servAddr; //server address
    rtp* frameToSend; //pointer to frame to send
    rtp* recFrame; //pointer to received frame
    char hostName[hostNameLength]; //string with host name
    struct timeval shortTimeout; //short timeout
    int numOfShortTimeouts = 0;//timeout counter
    int errorChance = 0; //error percentage number
    bool disconnect = false; //bool indicating disconnection

    //handling arguments to client
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

    //initiating CRC table
    initCrc();

    //preparing address to server
    prepareHostAddr(&servAddr, hostName, PORT);

    //state machine
    int state = CLOSED;
    while(1)
    {
        switch(state)
        {
            case CLOSED: //closed state, prepares socket and sends a syn to the server
                printf("Entered CLOSED state!\n");
                prepareSocket(&sfd, &myAddr);
                frameToSend = newFrame(SYN,0,0);
                sendFrame(sfd, frameToSend, servAddr, errorChance);
                printf("Syn sent!\n");
                free(frameToSend);
                state = SYN_SENT;
                break;

            case SYN_SENT: //syn sent state, waits for syn ack
                printf("Entered SYN_SENT state!\n");
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++) //long timeout counter
                {
                    resetShortTimeout(&shortTimeout);
                    if(waitForFrame(sfd, &shortTimeout) == 0) // short timeout, resend syn
                    {
                        frameToSend = newFrame(SYN,0,0);
                        sendFrame(sfd, frameToSend, servAddr, errorChance);
                        free(frameToSend);
                        printf("Resent SYN.\n");
                    }
                    else //frame to read
                    {
                        recFrame = receiveFrame(sfd, &servAddr);
                        if(recFrame->flags == SYN+ACK) //received frame is a syn+ack
                        {
                            printf("SYN+ACK received!\n");
                            printf("Sending ACK!\n");
                            frameToSend = newFrame(ACK,0,0);
                            sendFrame(sfd, frameToSend, servAddr, errorChance);
                            free(frameToSend);
                            free(recFrame);
                            state = PRECAUTION;
                            break; //break for loop
                        }
                        else
                        {
                            printf("Unexpected packet received! \n");
                            free(recFrame);
                        }

                    }
                }
                if(state!=PRECAUTION) //if state has not changed, long time out!
                {
                    printf("Long timeout! State CLOSED.\n");
                    close(sfd);
                    state = CLOSED;
                    disconnect = true;
                }
                break;


            case PRECAUTION: //precaution state, waits a second to make sure the ack has arrived
                printf("Entered PRECAUTION state!\n");
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    shortTimeout.tv_sec = 1; //setting short timeout to one second
                    shortTimeout.tv_usec = 0;
                    if(waitForFrame(sfd,&shortTimeout) == 0) //short timeout, change state to established
                    {
                        state = ESTABLISHED;
                        break;
                    }
                    else
                    {
                        recFrame = receiveFrame(sfd, &servAddr);
                        if(recFrame->flags == SYN+ACK) //syn+ack recieved again, resend ack
                        {
                            printf("SYN+ACK received!\n");
                            frameToSend = newFrame(ACK,0,0);
                            sendFrame(sfd, frameToSend, servAddr, errorChance);
                            free(frameToSend);
                            free(recFrame);
                            printf("Resent ACK.\n");
                        }
                        else
                        {
                            printf("Unexpected packet received!\n");
                            free(recFrame);
                        }
                    }
                }
                if(state!=ESTABLISHED) //if state has not changed, long time out!
                {
                    printf("Long timeout! State CLOSED.\n");
                    close(sfd);
                    state = CLOSED;
                    disconnect = true;
                }
                break;

            case ESTABLISHED: //established state, ready to send data
                printf("Entered ESTABLISHED state!\n");
                if(clientSlidingWindow(sfd, &servAddr, errorChance) == 0) // if sliding window returns 0, start teardown
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
                    disconnect = true;
                }
                break;

            case FIN_SENT: //fin sent state, waiting for ack
                printf("FIN sent!\n");
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    resetShortTimeout(&shortTimeout);

                    if(waitForFrame(sfd, &shortTimeout) == 0) // short timeout, Resend FIN
                    {
                        frameToSend = newFrame(FIN,0,0);
                        sendFrame(sfd, frameToSend, servAddr, errorChance);
                        printf("FIN resent!\n");
                        free(frameToSend);
                    }
                    else //frame to read
                    {
                        recFrame = receiveFrame(sfd, &servAddr);

                        if(recFrame->flags == ACK)/*received ACK*/
                        {
                            printf("ACK received!\n");
                            state = AWAIT_FIN;
                            free(recFrame);
                            break;
                        }
                        else /*received unexpected packet*/
                        {
                            printf("Unexpected packet received! \n");
                            free(recFrame);
                        }
                    }
                }/*End of for-loop*/
                if(state!=AWAIT_FIN)
                {
                    printf("Long timeout! Closing socket.\n");
                    close(sfd);
                    state = CLOSED;
                    disconnect = true;
                }
                break;

            case AWAIT_FIN: //await fin state, send ack when fin is received
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    resetShortTimeout(&shortTimeout);

                    if(waitForFrame(sfd, &shortTimeout) == 0) {} // short timeout, do nothing
                    else //frame to read
                    {
                        recFrame = receiveFrame(sfd, &servAddr);

                        if(recFrame->flags == FIN)/*received FIN*/
                        {
                            printf("FIN received!\n");
                            frameToSend = newFrame(ACK,0,0);
                            sendFrame(sfd, frameToSend, servAddr, errorChance);
                            printf("ACK sent!\n");
                            free(frameToSend);
                            state = SHORT_WAIT;
                            free(recFrame);
                            break;
                        }
                        else /*received unexpected frame*/
                        {
                            printf("Unexpected packet received! \n");
                            free(recFrame);
                        }
                    }
                }/*End of for-loop*/
                if(state!=SHORT_WAIT)
                {
                    printf("Long timeout! Closing socket.\n");
                    close(sfd);
                    state = CLOSED;
                    disconnect = true;
                }
                break;

            case SHORT_WAIT: //short wait state, close socket after a short wait
                usleep(200000);
                close(sfd);
                state = CLOSED;
                printf("Connection was torn down!\n");
                disconnect = true;
                break;

            default: // default state if catches state change errors
                perror("Undefined state.");
                exit(EXIT_FAILURE);
                break;
        }
        if(disconnect)//when disconnect is true, wait for keyboard input before reconnecting
        {
            printf("Press any key to reconnect.\n");
            getchar();
            disconnect = false;
        }
    }
    return 0;
}

