/*
Datacommunication Spring 2014
Lab 3 - Reliable Transportation Protocol
Students: sdn08003
          lja08001
*/

#include "server.h"
#include "protocol_std.h"

int main(int argc, char **argv)
{
    srand (time(NULL));

    int state = CLOSED;
    int fd;                             /* our socket (filedescriptor)*/
    struct sockaddr_in myaddr;          /* server address */
    struct sockaddr_in remaddr;         /* client address */

    int returnval;                      /*for checking returnval on select()*/
    char msgBuffer[MAXMSG];             /*buffer containing completed message*/
    int frameCountdown = 0;             /*counter for amount of frames yet to be received*/
    int expectedSeqence = 0;

    struct timeval shortTimeout;
    int numOfshortTimeouts = 0;         /*iteration konstant for tiemouts*/
    int chanceOfFrameError = 0;

    rtp* frame = NULL;                  /*a pointer, used to read and assign new frames to*/

    /*create a list of all possible crc8 remainders for fasterer computation*/
    initCrc();

    /*get probability for how freqently errors will occur*/
    chanceOfFrameError = getFrameErrorPercentage();

    while(1)/*press Ctr + 'c' to exit program*/
    {
        switch(state)/*statemachine connection setup server*/
        {
            case CLOSED:
            {
                close(fd);/*Close (if any) old socket*/
                printf("\n");
                printf("Press any key to open socket. \n");
                getchar();
                prepareSocket(&fd, &myaddr);/*create and bind socket*/
                state = LISTEN;
                break;

            }/*End of case CLOSED*/

            case LISTEN:
            {
                printf("Awaiting SYN \n");

                /*the for-loop represents a long time out, and one iteration represent a short time out*/
                for(numOfshortTimeouts=0; numOfshortTimeouts<longTimeOut; numOfshortTimeouts++)
                {
                    resetShortTimeout(&shortTimeout);
                    returnval = waitForFrame(fd,&shortTimeout);

                    if(returnval == 0)/*short timeout*/
                    {
                        /*Nothing to read/Nothing to be done*/
                    }
                    else/*there is something to read on the filedescriptor*/
                    {
                        frame = receiveFrame(fd, &remaddr);

                        if (frame->flags == SYN)/*expected frame received */
                        {
                            printf("SYN received \n");
                            free(frame);
                            frame = newFrame(SYN+ACK, 0, 0);//create a SYN+ACK
                            sendFrame(fd, frame, remaddr, chanceOfFrameError);
                            printf("SYN+ACK sent  \n");
                            state = SYN_RECEIVED;
                            free(frame);
                            break;
                        }/*End of expected frame received*/
                        else/*unexpected packet recieved*/
                        {
                            printf("Unexpected frame received. seq: %d. Throw away \n", frame->seq);
                            free(frame);

                        }/*End of unexpected frame received*/
                    }/*End of something to read*/
                }/*End of for-loop*/

                if(state != SYN_RECEIVED)
                {
                /*Long time out triggered! Reset connection setup*/
                printf("Long timeout. Reset connection setup \n");
                state = CLOSED;
                break;
                }
            }/*End of case LISTEN*/

            case SYN_RECEIVED:
            {
                printf("Awaiting ACK \n");

                /*the for-loop represents a long time out, and one iteration represent a short time out*/
                for(numOfshortTimeouts=0; numOfshortTimeouts<longTimeOut; numOfshortTimeouts++)
                {
                    resetShortTimeout(&shortTimeout);
                    returnval = waitForFrame(fd,&shortTimeout);

                    if(returnval == 0)/*short timeout*/
                    {
                        /*resend syn+ack*/
                        frame = newFrame(SYN+ACK, 0, 0);//create a SYN+ACK frame
                        sendFrame(fd, frame, remaddr, chanceOfFrameError);
                        printf("SYN+ACK resent \n");
                        free(frame);

                    }/*short timeout, no request to connect received*/
                    else/*we got something to read*/
                    {
                        frame = receiveFrame(fd, &remaddr);

                        /*expected packet received */
                        if (frame->flags == ACK)
                        {
                            printf("ACK received \n");
                            state = ESTABLISHED;
                            free(frame);
                            break;
                        }
                        /*unexpected packet received, throw away packet, keep listening */
                        else if(frame->flags == SYN)
                        {
                            printf("SYN received \n");
                            frame = newFrame(SYN+ACK, 0, 0);//create a SYN+ACK frame
                            sendFrame(fd, frame, remaddr, chanceOfFrameError); /*resend SYN+ACK*/
                            printf("SYN+ACK resent \n");
                            free(frame);
                        }
                        else
                        {
                            printf("Unexpected frame received. seq: %d. Trow away \n",frame->seq );
                            free(frame);
                        }
                    }/*End of something to read*/
                }/*End of for-loop*/

                if(state != ESTABLISHED)
                {
                    printf("Long timeout. Reset connection setup \n");
                    state = CLOSED;
                }
                else
                {
                    printf("Connection Established \n\n");
                    expectedSeqence = 0;
                }

                break;

            }/*End of CASE SYN_RECEIVED*/

            case ESTABLISHED:
            {
                printf("Awating INF on seq: %d \n",expectedSeqence );
                int strInd = 0;

                for(numOfshortTimeouts=0; numOfshortTimeouts<longTimeOut; numOfshortTimeouts++)
                {
                    resetShortTimeout(&shortTimeout);
                    returnval = waitForFrame(fd,&shortTimeout);

                    if(returnval == 0)/*short timeout*/
                    {

                    }
                    else/*we got something to read*/
                    {
                        numOfshortTimeouts =0;
                        frame = receiveFrame(fd, &remaddr);

                        /*FIN received*/
                        if(frame->flags == FIN)/*receive request to teardown connection*/
                        {
                            break;
                        }
                        /*INF received*/
                        else if(frameCountdown == 0 && (frame->flags == INF) && (frame->seq == expectedSeqence) )/*receive info on next msglength */
                        {
                            printf("INF received. seq %d \n",frame->seq);
                            printf("Number of frames yet to be received = %d \n", (unsigned char)frame->data);
                            frameCountdown = (unsigned char)frame->data;
                            free(frame);
                            frame = newFrame(ACK, expectedSeqence, 0);//create ACK
                            sendFrame(fd, frame, remaddr, chanceOfFrameError);
                            printf("ACK sent on seq: %d \n", expectedSeqence);
                            free(frame);
                            expectedSeqence = ((expectedSeqence + 1) % MAXSEQ);/*update window*/
                        }
                        /*expected received*/
                        else if (frame->flags == ACK &&
                                 frame->seq == expectedSeqence &&
                                 frameCountdown > 1 )
                        {
                            printf("Expected frame received. seq %d \n", frame->seq);

                            /*put char in msgbuffer*/
                            msgBuffer[strInd] = frame->data;
                            strInd++;
                            frameCountdown--;
                            free(frame);
                            frame = newFrame(ACK, expectedSeqence, 0);//create ACK
                            sendFrame(fd, frame, remaddr, chanceOfFrameError);
                            printf("ACK sent on seq: %d \n", expectedSeqence);
                            expectedSeqence = ((expectedSeqence + 1) % MAXSEQ);
                            free(frame);
                        }
                        /*final frame received*/
                        else if (frame->flags == ACK &&
                                 frame->seq == expectedSeqence &&
                                 frameCountdown == 1)
                        {
                            printf("Final frame received. seq : %d \n", expectedSeqence);
                            msgBuffer[strInd] = frame->data;
                            frameCountdown--;
                            free(frame);
                            frame = newFrame(ACK, expectedSeqence, 0);//create ACK
                            sendFrame(fd, frame, remaddr, chanceOfFrameError);
                            printf("Final ACK sent. seq: %d \n\n", expectedSeqence);
                            free(frame);
                            printf("_____________________________________________________________________ \n");
                            printf("Received message: %s \n\n",msgBuffer);
                            printf("_____________________________________________________________________ \n\n");
                            strInd = 0;
                            expectedSeqence = ((expectedSeqence + 1) % MAXSEQ);
                            printf("Listening for new INF on seq: %d \n",expectedSeqence );

                        }/*unexpected frame out of order or outside window*/
                        else if(frame->flags == ACK &&
                                frame->seq != expectedSeqence)
                        {
                            int i;

                            for(i=expectedSeqence; i!=(expectedSeqence+WINDSIZE)%MAXSEQ; i=(i+1)%MAXSEQ)
                            {
                                if(frame->seq == i)
                                {
                                    //frame out of order.
                                    printf("Received frame seq: %d. Out of order. Expecting seq: %d  \n",frame->seq, expectedSeqence);
                                    break;
                                }
                            }
                            if(i==(expectedSeqence+WINDSIZE)%MAXSEQ)
                            {
                                //Frame outside window. Send ACK on corresponding sequence*/
                                printf("Received frame outside window! Seq: %d\n",frame->seq);
                                rtp* tempframe = newFrame(ACK, frame->seq, 0);//create ACK
                                sendFrame(fd, tempframe, remaddr, chanceOfFrameError);
                                printf("ACK sent seq: %d \n", tempframe->seq);
                                free(tempframe);
                            }

                            free(frame);
                        }
                        else/*received unexpected frame, throw away*/
                        {
                            printf("Unexpected frame received. seq: %d. Throw away \n",frame->seq);
                            free(frame);
                        }
                    }/*End of frame to read*/
                }/*End of for-loop*/
                if(numOfshortTimeouts == longTimeOut)
                {
                    printf("Long timeout. Reset connection setup \n");
                    state = CLOSED;
                    break;
                }
                else if(frame->flags == FIN)
                {
                    state = RESPOND_TEARDOWN;
                    free(frame);
                    break;
                }
            }/*End of CASE ESTABLISHED*/

            case RESPOND_TEARDOWN:
            {
                printf("Request to close socket received \n");
                state = teardownResponse(fd, &shortTimeout, &remaddr, chanceOfFrameError);
                break;
            }/*End of CASE RESPOND_TEARDOWN*/
        }/*End of switch*/
    }/*End of while(1)*/

    return 0;
}
