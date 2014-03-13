#include "server.h"

int teardownResponse(int fd, struct timeval* shortTimeout, struct sockaddr_in* sourceAddr, int chanceOfFrameError)
{
    printf("Teardownsequence initiated. \n");
    /*TODO Application close*/

    rtp* receivedFrame = NULL;
    rtp* frameToSend = NULL;
    int numOfShortTimeouts = 0;
    int applicationCloseDelay = 5;/* about 5 x 200ms = 1 sek delay*/
    int state = AWAIT_CLOSE;

    frameToSend = newFrame(ACK, 0, 0);//create a ACK frame
    sendFrame(fd, frameToSend, *sourceAddr, chanceOfFrameError); /*send ACK*/
    printf("ACK sent \n");
    free(frameToSend);

    while(1)
    {
        switch(state)
        {
            case AWAIT_CLOSE:
            {
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    resetShortTimeout(shortTimeout);

                    if(waitForFrame(fd, shortTimeout) == 0)// short timeout,
                    {
                        if(applicationCloseDelay == 0)//give 1 second delay so application will get a chance to close
                        {
                            printf("Application Closed. \n");
                            state = AWAIT_ACK;
                            break;
                        }
                        applicationCloseDelay --;
                    }

                    else //frame to read
                    {
                        receivedFrame = receiveFrame(fd, sourceAddr);

                        if(receivedFrame->flags == FIN)/*received expected packet FIN, resend ACK*/
                        {
                            printf("FIN received \n");
                            frameToSend = newFrame(ACK, 0, 0);//create a ACK frame
                            sendFrame(fd, frameToSend, *sourceAddr, chanceOfFrameError); /*resend ACK*/
                            printf("ACK resent \n");
                            free(frameToSend);
                        }
                        else /*received unexpected packet*/
                        {
                            printf("Unexpected packet received: Throw away! \n");
                            free(receivedFrame);
                        }
                    }
                }/*End of for-loop*/
                if(state==AWAIT_ACK)
                {
                    break;
                }

                printf("Long timeout! Closing socket.\n");
                state = CLOSED;
                return state;

            }/*End of case AWAIT_CLOSE*/

            case AWAIT_ACK:
            {
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    resetShortTimeout(shortTimeout);

                    if(waitForFrame(fd, shortTimeout) == 0) // short timeout, resend fin
                    {
                        frameToSend = newFrame(FIN, 0, 0);//create a FIN frame
                        sendFrame(fd, frameToSend, *sourceAddr, chanceOfFrameError);
                        printf("FIN resent \n");
                        free(frameToSend);

                    }
                    else //frame to read
                    {
                        receivedFrame = receiveFrame(fd, sourceAddr);

                        if(receivedFrame->flags == FIN)/*received expected packet FIN*/
                        {
                            printf("FIN received!\n");
                            frameToSend = newFrame(ACK, 0, 0);//create a ACK frame
                            sendFrame(fd, frameToSend, *sourceAddr, chanceOfFrameError);
                            printf("ACK resent \n");
                            free(frameToSend);
                        }
                        else if(receivedFrame->flags == ACK)/*received expected packet ACK*/
                        {
                            printf("ACK received!\n");
                            free(receivedFrame);
                            state = CLOSED;
                            break;
                        }
                        else /*received unexpected packet*/
                        {
                            printf("Unexpected packet received: throw away! \n");
                            free(receivedFrame);
                        }
                    }
                }/*End of for-loop*/
                if(state==CLOSED)
                {
                    printf("Socket closed successfully \n");
                    return state;
                }
                printf("Long timeout! Closing socket.\n");
                state = CLOSED;
                return state;

            }/*End of case AWAIT_ACK*/
        }/*End of switch*/
    }/*End of while*/

}




