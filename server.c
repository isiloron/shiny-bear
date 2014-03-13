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

    struct timeval shortTimeout1;
    struct timeval shortTimeout2;
    int numOfshortTimeouts = 1; /*iteration konstant for tiemouts*/
    int chanceOfFrameError = 0;
    bool dataIsComplete = false;
    bool dataToReceive = false;
    rtp* frame = NULL;

    system("clear");

    while(1)/*for now press Ctr + 'c' to exit program*/
    {
        switch(state)
        {
            case CLOSED:
            {
                chanceOfFrameError = getFrameErrorPercentage();

                close(fd);/*Close (if any) old socket*/

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
                    resetShortTimeout(&shortTimeout1);

                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set for reading*/

                    returnval = select(fd + 1, &read_fd_set, NULL, NULL, &shortTimeout1);/*wait for something to be read on the filedescriptor in the set*/

                    if(returnval == -1)/*ERROR*/
                    {
                        printf("Select failed\n");
                        exit(0);
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
                    resetShortTimeout(&shortTimeout1);

                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set*/

                    /*wait for something to be read on the filedescriptor in the set*/
                    returnval = select(fd + 1, &read_fd_set, NULL, NULL, &shortTimeout1);

                    if(returnval == -1)/*ERROR*/
                    {
                        perror("Select failed \n");
                        exit(0);
                    }
                    else if(returnval == 0){}/*short timeout, no request to connect received*/
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
                printf("Awaiting new message... \n");

                dataToReceive = false;
                dataIsComplete = false;

                for(numOfshortTimeouts=0; numOfshortTimeouts<longTimeOut; numOfshortTimeouts++)
                {
                    printf("Seq: %d expected \n",expectedSeqence);

                    resetShortTimeout(&shortTimeout1);

                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set*/

                    /*Await client to send something*/
                    returnval = select(fd + 1, &read_fd_set, NULL, NULL, &shortTimeout1);

                    if(returnval == -1)/*ERROR*/
                    {
                        perror("Select failed \n");
                        exit(0);
                    }
                    else if(returnval == 0)/*short timeout, no request to connect received*/
                    {

                    }
                    else/*we got something to read*/
                    {
                        frame = receiveFrame(fd, &remaddr);

                        if(frame->flags == FIN)/*receive request to teardown connection*/
                        {
                            state = RESPOND_TEARDOWN;
                            free(frame);
                            break;
                        }
                        else if( (frame->flags == INF) && (frame->seq == expectedSeqence) )/*receive info on next msglength */
                        {
                            printf("INF received, with seq %d \n",frame->seq);
                            dataToReceive = true;

                            frameCountdown = frame->data;
                            free(frame);

                            /*Send ACK on corresponding frame sequence*/
                            frame = newFrame(ACK, expectedSeqence, 0);//create ACK
                            sendFrame(fd, frame, remaddr, chanceOfFrameError); /*send ACK*/
                            printf("Sent ACK on seq: %d \n", expectedSeqence);
                            free(frame);

                            expectedSeqence = ((expectedSeqence + 1) % MAXSEQ);

                            int strInd = 0;

                            for(numOfshortTimeouts=0; numOfshortTimeouts<longTimeOut; numOfshortTimeouts++)/*receive msg until completion*/
                            {
                                resetShortTimeout(&shortTimeout2);

                                FD_ZERO(&read_fd_set);/*clear set*/
                                FD_SET(fd,&read_fd_set);/*put the fd in the set*/

                                /*Await client to send something*/
                                returnval = select(fd + 1, &read_fd_set, NULL, NULL, &shortTimeout2);

                                if(returnval == -1)/*ERROR*/
                                {
                                    perror("Select failed \n");
                                    exit(0);
                                }
                                else if(returnval == 0){}/*short timeout, no frame received*/
                                else/*we got something to read*/
                                {
                                    frame = receiveFrame(fd, &remaddr);

                                    if((frame->flags == ACK) &&
                                       (frame->seq == expectedSeqence) &&
                                       (frameCountdown != 1) )/*received next expected frame */
                                    {
                                        /*put char in msgbuffer*/
                                        msgBuffer[strInd] = frame->data;
                                        strInd++;
                                        frameCountdown--;
                                        free(frame);

                                        /*Send ACK on corresponding sequence*/
                                        frame = newFrame(ACK, expectedSeqence, 0);//create ACK
                                        sendFrame(fd, frame, remaddr, chanceOfFrameError); /*resend ACK*/
                                        printf("Sent ACK on seq: %d \n", expectedSeqence);
                                        expectedSeqence = ((expectedSeqence + 1) % MAXSEQ);
                                        free(frame);
                                    }
                                    else if(frameCountdown == 1)/*last frame to read, data is complete, print msg from msgbuffer to screen*/
                                    {
                                        printf("Received msg: ");

                                        msgBuffer[strInd] = frame->data;

                                        printf("%s",msgBuffer);

                                        free(frame);
                                        strInd = 0;
                                        expectedSeqence = ((expectedSeqence + 1) % MAXSEQ);
                                        dataIsComplete =true;
                                        break;
                                    }
                                    else/*received unexpected frame, throw away*/
                                    {
                                        printf("Unexpected frame received. Throw away seq: %d\n",frame->seq);
                                        free(frame);
                                    }
                                }/*End of got ACK frames to read*/
                            }/*End of for-loop send until message completion*/

                            if(dataIsComplete == false)
                            {
                                printf("Long timeout. Msg was incomplete. Reset connection setup \n");
                                state = CLOSED;
                                break;
                            }
                        }/*End of recieved info-frame*/
                    }/*End of got INF frame to read*/
                }/*End of for-loop*/

                if(dataToReceive ==false)
                {
                    printf("Long timeout. No INF-frame received. Reset connection setup \n");
                    state = CLOSED;
                    break;
                }
                else/*msg complete. Listen for new msg*/
                {
                    state = ESTABLISHED;
                    break;
                }

            }/*End of CASE ESTABLISHED*/

            case RESPOND_TEARDOWN:
            {
                printf("Request to close socket received \n");
                state = teardownResponse(fd, &shortTimeout1, &remaddr, chanceOfFrameError);

                break;
            }/*End of CASE RESPOND_TEARDOWN*/
        }/*End of switch*/
    }/*End of while(1)*/

    return 0;
}
