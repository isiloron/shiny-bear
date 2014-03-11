#include "server.h"
/*frame is not a reasonable name, cuz it is not a frame! Rename it to "frame" or something like that:P*/
int main(int argc, char **argv)
{
    int state = CLOSED;
    fd_set read_fd_set; /*create a set for sockets*/
    int fd;/* our socket (filedescriptor)*/
    struct sockaddr_in myaddr;/* server address */
    struct sockaddr_in remaddr;/* remote address */
    int returnval; /*for checking returnval on select()*/

    struct timeval shortTimeout;
    int numOfshortTimeouts = 0; /*iteration konstant for tiemouts*/
    pthread_t readThreadId;

    rtp* frame = NULL;


    while(1)/*for now press Ctr + 'c' to exit program*/
    {
        switch(state)
        {
            case CLOSED:
            {
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
                    resetShortTimeout(&shortTimeout);

                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set for reading*/

                    returnval = select(fd + 1, &read_fd_set, NULL, NULL, &shortTimeout);/*wait for something to be read on the filedescriptor in the set*/

                    if(returnval == -1)/*ERROR*/
                    {
                        printf("Select failed!\n");
                        exit(0);
                    }
                    else if(returnval == 0){}/*short timeout, no request to connect received*/
                    else/*there is something to read on the filedescriptor*/
                    {
                        frame = receiveFrame(fd, &remaddr);

                        if (frame->flags == SYN)/*expected frame received */
                        {
                            printf("Received SYN\n");

                            free(frame);

                            frame = newFrame(SYN+ACK, 0, 0, 0);//create a SYN+ACK frame

                            sendFrame(fd, frame, remaddr);

                            printf("Sent SYN + ACK\n");

                            state = SYN_RECEIVED;/*Move to next state*/

                            /*delete the created frame */
                            free(frame);

                            break;
                        }/*End of expected frame received*/
                        else/*unexpected packet recieved*/
                        {
                            printf("Unexpected frame received. Throw away!\n");
                            /*throw away frame, keep listening*/

                            /*delete the created frame */
                            free(frame);

                        }/*End of unexpected frame received*/
                    }/*End of something to read*/
                }/*End of for-loop*/

                if(state != SYN_RECEIVED)
                {
                /*Long time out triggered! Reset connection setup*/
                printf("Long timeout!\n");
                state = CLOSED;

                break;
                }
            }/*End of case LISTEN*/

            case SYN_RECEIVED:
            {
                printf("Awaiting ACK...\n");

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
                        perror("Select failed!\n");
                        exit(0);
                    }
                    else if(returnval == 0){}/*short timeout, no request to connect received*/
                    else/*we got something to read*/
                    {
                        frame = receiveFrame(fd, &remaddr);

                        /*expected packet received */
                        if (frame->flags == ACK)
                        {
                            printf("Received ACK!\n");

                            state = ESTABLISHED;

                            /*delete the created frame */
                            free(frame);

                            break;
                        }
                        /*unexpected packet received, throw away packet, keep listening */
                        else if(frame->flags == SYN)
                        {
                                frame = newFrame(SYN+ACK, 0, 0, 0);//create a SYN+ACK frame
                                sendFrame(fd, frame, remaddr); /*resend SYN+ACK*/
                                printf("Resent SYN + ACK\n");
                                free(frame);
                        }
                        else
                        {
                            printf("Unexpected packet received! Trow away!\n");
                            /*delete the created frame */
                            free(frame);
                        }
                    }/*End of something to read*/
                }/*End of for-loop*/

                if(state != ESTABLISHED)
                {
                    printf("Long timeout! Reset connection setup\n");
                    state = CLOSED;
                }
                break;

            }/*End of CASE SYN_RECEIVED*/

            case ESTABLISHED:
            {
                printf("Connection established!\n");

//                if(pthread_create(&readThreadId,NULL,&readMessageFromClient,&fd)!=0)
//                {
//                    printf("Could not create reading thread! \n");
//                    exit(0);
//                }

                while(1)
                {
                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set*/

                    /*Await client to send something, forevA*/
                    select(fd + 1, &read_fd_set, NULL, NULL, NULL);

                    frame = receiveFrame(fd, &remaddr);

                    if(frame->flags == FIN)
                    {
                        state = RESPOND_TEARDOWN;
                        free(frame);
                        break;
                    }
                }

                break;
            }/*End of CASE ESTABLISHED*/

            case RESPOND_TEARDOWN:
            {
                printf("Request to close socket received \n");
                state = teardownResponse(fd, &shortTimeout, &remaddr);

                break;
            }/*End of CASE RESPOND_TEARDOWN*/
        }/*End of switch*/
    }/*End of while(1)*/

    return 0;
}
