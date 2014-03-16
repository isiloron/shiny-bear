/*
Data communication Spring 2014
Lab 3 - Reliable Transportation Protocol
Students: sdn08003
          lja08001
*/

#include "server.h"

int getFrameErrorPercentage()/*read the user input that will become the errorpercentage*/
{
    char buffer[10];
    int percentage;

    printf("Enter the probability, in percentage (0-100), that a frame will be currupted/lost: ");
    fflush(stdin);
    fgets(buffer, 10, stdin);
    percentage = atoi(buffer);

    if(percentage < 0)
        percentage = 0;
    if(percentage > 100)
        percentage = 100;

    printf("Errorpercentage %d \n", percentage);
    return percentage;
}

int teardownResponse(int fd, struct timeval* shortTimeout, struct sockaddr_in* sourceAddr, int chanceOfFrameError)
{
    printf("Teardownsequence initiated. \n\n");

    rtp* receivedFrame = NULL;
    rtp* frameToSend = NULL;
    int numOfShortTimeouts = 0;
    int applicationCloseDelay = 5;/* about 5 x 200ms = 1 sek delay*/
    int state = AWAIT_CLOSE;

    /*send ack to confirm teardown start*/
    frameToSend = newFrame(ACK, 0, 0);//create a ACK
    printf("Sending ACK \n");
    sendFrame(fd, frameToSend, *sourceAddr, chanceOfFrameError);
    free(frameToSend);

    while(1)
    {
        switch(state)/*statemachine connection teardown*/
        {
            case AWAIT_CLOSE:
            {
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    resetShortTimeout(shortTimeout);

                    if(waitForFrame(fd, shortTimeout) == 0)// short timeout,
                    {
                        if(applicationCloseDelay == 0)//give about 1 second delay so application will get a chance to close
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

                        if(receivedFrame->flags == FIN)/*received FIN, resend ACK*/
                        {
                            printf("FIN received \n");
                            frameToSend = newFrame(ACK, 0, 0);//create a ACK frame
                            printf("Sending ACK \n");
                            sendFrame(fd, frameToSend, *sourceAddr, chanceOfFrameError);
                            free(frameToSend);
                        }
                        else /*received unexpected packet*/
                        {
                            printf("Unexpected packet received. seq: %d. Throw away. \n", receivedFrame->seq);
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
                        frameToSend = newFrame(FIN, 0, 0);//create a FIN
                        printf("Resending FIN \n");
                        sendFrame(fd, frameToSend, *sourceAddr, chanceOfFrameError);
                        free(frameToSend);
                    }
                    else //frame to read
                    {
                        receivedFrame = receiveFrame(fd, sourceAddr);

                        if(receivedFrame->flags == FIN)/*received FIN*/
                        {
                            printf("FIN received!\n");
                            frameToSend = newFrame(ACK, 0, 0);//create a ACK
                            printf("Resending ACK \n");
                            sendFrame(fd, frameToSend, *sourceAddr, chanceOfFrameError);
                            free(frameToSend);
                        }
                        else if(receivedFrame->flags == ACK)/*received ACK*/
                        {
                            printf("ACK received!\n");
                            free(receivedFrame);
                            state = CLOSED;

                            break;
                        }
                        else /*received unexpected packet*/
                        {
                            printf("Unexpected packet received. seq: %d. Throw away. \n", receivedFrame->seq);
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
