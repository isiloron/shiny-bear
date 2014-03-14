#include "server.h"
#include "protocol_std.h"

int main(int argc, char **argv)
{

    unsigned int seed = (unsigned int)time(NULL);
    srand (seed);

    int state = CLOSED;
    fd_set read_fd_set; /*create a set for sockets*/
    int fd;/* our socket (filedescriptor)*/
    struct sockaddr_in myaddr;/* server address */
    struct sockaddr_in remaddr;/* remote address */
    int returnval; /*for checking returnval on select()*/
    char msgBuffer[MAXMSG];
    int frameCountdown = 0;
    int expectedSeqence = 0;

    struct timeval shortTimeout;
    int numOfshortTimeouts = 1; /*iteration konstant for tiemouts*/
    int chanceOfFrameError = 0;
    rtp* frame = NULL;

    while(1)/*for now press Ctr + 'c' to exit program*/
    {
        switch(state)
        {
            case CLOSED:
            {
                close(fd);/*Close (if any) old socket*/

                chanceOfFrameError = getFrameErrorPercentage();

                prepareSocket(&fd, &myaddr);/*create and bind socket*/

                state = LISTEN;

                break;

            }/*End of case CLOSED*/

            case LISTEN:
            {
                printf("Awaiting SYN...\n");

                /*the for-loop represents a long time out, and one iteration represent a short time out*/
                for(numOfshortTimeouts=0; numOfshortTimeouts<longTimeOut; numOfshortTimeouts++)
                {
                    resetShortTimeout(&shortTimeout);

                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set for reading*/

                    returnval = select(fd + 1, &read_fd_set, NULL, NULL, &shortTimeout);/*wait for something to be read on the filedescriptor in the set*/

                    if(returnval == -1)/*ERROR*/
                    {
                        printf("Select failed\n");
                        exit(EXIT_FAILURE);
                    }
                    else if(returnval == 0){}/*short timeout, no request to connect received*/
                    else/*there is something to read on the filedescriptor*/
                    {
                        frame = receiveFrame(fd, &remaddr);

                        if (frame->flags == SYN)/*expected frame received */
                        {
                            printf("Received SYN \n");

                            free(frame);

                            frame = newFrame(SYN+ACK, 0, 0);//create a SYN+ACK frame

                            sendFrame(fd, frame, remaddr, chanceOfFrameError);

                            printf("Sent SYN + ACK \n");

                            state = SYN_RECEIVED;/*Move to next state*/

                            /*delete the created frame */
                            free(frame);

                            break;
                        }/*End of expected frame received*/
                        else/*unexpected packet recieved*/
                        {
                            printf("Unexpected frame received. Throw away \n");
                            /*throw away frame, keep listening*/

                            /*delete the created frame */
                            free(frame);

                        }/*End of unexpected frame received*/
                    }/*End of something to read*/
                }/*End of for-loop*/

                if(state != SYN_RECEIVED)
                {
                /*Long time out triggered! Reset connection setup*/
                printf("Long timeout \n");
                state = CLOSED;

                break;
                }
            }/*End of case LISTEN*/

            case SYN_RECEIVED:
            {
                printf("Awaiting ACK... \n");

                /*the for-loop represents a long time out, and one iteration represent a short time out*/
                for(numOfshortTimeouts=0; numOfshortTimeouts<longTimeOut; numOfshortTimeouts++)
                {
                    resetShortTimeout(&shortTimeout);

                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set*/

                    /*wait for something to be read on the filedescriptor in the set*/
                    returnval = select(fd + 1, &read_fd_set, NULL, NULL, &shortTimeout);

                    if(returnval == -1)/*ERROR*/
                    {
                        perror("Select failed \n");
                        exit(0);
                    }
                    else if(returnval == 0)/*short timeout, no request to connect received*/
                    {
                        /*resend syn+ack*/
                        frame = newFrame(SYN+ACK, 0, 0);//create a SYN+ACK frame

                        sendFrame(fd, frame, remaddr, chanceOfFrameError);

                        printf("Resent SYN + ACK \n");
                        free(frame);

                    }/*short timeout, no request to connect received*/
                    else/*we got something to read*/
                    {
                        frame = receiveFrame(fd, &remaddr);

                        /*expected packet received */
                        if (frame->flags == ACK)
                        {
                            printf("Received ACK \n");

                            state = ESTABLISHED;

                            /*delete the created frame */
                            free(frame);

                            break;
                        }
                        /*unexpected packet received, throw away packet, keep listening */
                        else if(frame->flags == SYN)
                        {
                                frame = newFrame(SYN+ACK, 0, 0);//create a SYN+ACK frame
                                sendFrame(fd, frame, remaddr, chanceOfFrameError); /*resend SYN+ACK*/
                                printf("Resent SYN + ACK \n");
                                free(frame);
                        }
                        else
                        {
                            printf("Unexpected packet received. Trow away \n");
                            /*delete the created frame */
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
                    printf("Connection established \n");
                    expectedSeqence = 0;
                }

                break;

            }/*End of CASE SYN_RECEIVED*/

            case ESTABLISHED:
            {
                printf("Listening for new INF on seq: %d \n",expectedSeqence );
                int strInd = 0;
                for(numOfshortTimeouts=0; numOfshortTimeouts<longTimeOut; numOfshortTimeouts++)
                {
                    resetShortTimeout(&shortTimeout);

                    /*Await client to send something*/
                    returnval = waitForFrame(fd,&shortTimeout);
                    if(returnval == 0)/*short timeout, no request to connect received*/
                    {

                    }
                    else/*we got something to read*/
                    {
                        frame = receiveFrame(fd, &remaddr);

                        if(frame->flags == FIN)/*receive request to teardown connection*/
                        {
                            break;
                        }
                        else if(frameCountdown == 0 && (frame->flags == INF) && (frame->seq == expectedSeqence) )/*receive info on next msglength */
                        {
                            printf("INF received, with seq %d \n",frame->seq);

                            frameCountdown = frame->data;
                            free(frame);

                            /*Send ACK on corresponding frame sequence*/
                            frame = newFrame(ACK, expectedSeqence, 0);//create ACK
                            sendFrame(fd, frame, remaddr, chanceOfFrameError); /*send ACK*/
                            printf("Sent ACK on seq: %d \n", expectedSeqence);
                            free(frame);

                            expectedSeqence = ((expectedSeqence + 1) % MAXSEQ);
                            numOfshortTimeouts =0;
                        }
                        else if (frame->flags == ACK &&
                                 frame->seq == expectedSeqence &&
                                 frameCountdown > 1 )
                        {
                            printf("Expected data-frame received with seq %d \n", frame->seq);
                            /*put char in msgbuffer*/
                            msgBuffer[strInd] = frame->data;
                            strInd++;
                            frameCountdown--;
                            free(frame);

                            /*Send ACK on corresponding sequence*/
                            frame = newFrame(ACK, expectedSeqence, 0);//create ACK
                            sendFrame(fd, frame, remaddr, chanceOfFrameError); /*send ACK*/
                            printf("Sent ACK on seq: %d \n", expectedSeqence);
                            expectedSeqence = ((expectedSeqence + 1) % MAXSEQ);
                            free(frame);
                        }
                        else if (frame->flags == ACK &&
                                 frame->seq == expectedSeqence &&
                                 frameCountdown == 1)
                        {
                            printf("Last frame received!\n");
                            msgBuffer[strInd] = frame->data;
                            frameCountdown--;
                            free(frame);

                            /*Send ACK on corresponding sequence*/
                            frame = newFrame(ACK, expectedSeqence, 0);//create ACK
                            sendFrame(fd, frame, remaddr, chanceOfFrameError); /*send ACK*/
                            printf("Sent final ACK. seq: %d \n\n", expectedSeqence);
                            free(frame);

                            printf("Received msg: %s",msgBuffer);

                            strInd = 0;
                            expectedSeqence = ((expectedSeqence + 1) % MAXSEQ);

                            printf("Listening for new INF on seq: %d \n",expectedSeqence );
                            numOfshortTimeouts =0;
                        }
                        else if(frame->flags == ACK)
                        {
                            int i;
                            for(i=expectedSeqence; i!=(expectedSeqence+WINDSIZE)%MAXSEQ; i=(i+1)%MAXSEQ)
                            {
                                if(frame->seq == i)
                                {
                                    //frame out of order.
                                    printf("Frame out of order received. Throw away seq: %d \n",frame->seq);
                                    break;
                                }
                            }
                            if(i==(expectedSeqence+WINDSIZE)%MAXSEQ)
                            {
                                //outside window
                                /*Send ACK on corresponding sequence*/
                                printf("Received frame outside of window! Seq: %d\n",frame->seq);
                                rtp* tempframe = newFrame(ACK, frame->seq, 0);//create ACK
                                sendFrame(fd, tempframe, remaddr, chanceOfFrameError); /*send ACK*/
                                printf("Sent ACK seq: %d \n", tempframe->seq);
                                free(tempframe);
                            }
                            free(frame);
                        }
                        else/*received unexpected frame, throw away*/
                        {
                            printf("Unexpected frame received. Throw away seq: %d \n",frame->seq);
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
