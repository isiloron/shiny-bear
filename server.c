#include "server.h"
/*frame is not a reasonable name, cuz it is not a frame! Rename it to "frame" or something like that:P*/
int main(int argc, char **argv)
{
    int state = CLOSED;
    fd_set read_fd_set; /*create a set for sockets*/
    int fd;/* our socket (filedescriptor)*/
    struct sockaddr_in myaddr;/* our address */
    struct sockaddr_in remaddr;/* remote address */
    socklen_t addrlen = sizeof(remaddr);/* length of addresses */
    void* serializedSegment = malloc(sizeof(rtp)); /*mem addresses to hold serialized sequences*/
    struct Buffer* buf;/*fhelp struct for serializing and deserializing*/
    int returnval; /*for checking returnval on select()*/

    /*udp packet members*/
    int flags = 0;
    int seq = 0;
    int crc = 0;
    char data = 0;

    /*Timeouts*/
    int longTimeOut = 0;
    rtp* frame = NULL;
    struct timeval shortTimeout;
    int numOfshortTimeouts = 0; /*iteration konstant for tiemouts*/

    while(1)/*for now press Ctr + 'c' to exit program*/
    {
        switch(state)
        {
            case CLOSED:
            {
                close(fd);/*Close (if any) old socket*/

                fd = createSocket();/* Try to create and open up a new UDP socket */

                state = bind_UDP_Socket(&myaddr, fd);/* Try to bind the socket to any valid IP address*/

                break;

            }/*End of case CLOSED*/

            case LISTEN:
            {
                printf("Listening for SYN...\n");

                longTimeOut = 600;/*number of short timeouts that will correspond to one long timeout*/

                /*the for-loop represents a long time out, and one iteration represent a short time out*/
                for(numOfshortTimeouts = 0; numOfshortTimeouts < longTimeOut; numOfshortTimeouts ++)
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
                        printf("Reading something... \n");

                        recvfrom(fd, serializedSegment, sizeof(rtp), 0, (struct sockaddr *)&remaddr, &addrlen);/*received serialized segment*/

                        frame = newFrame(0, 0, 0, 0);/*create empty frame, to put serializedSegment in*/

                        buf = newBuffer();/*create helpbuffer for deserializing*/

                        buf->data = serializedSegment;/*put serialized segment in buffer*/

                        deserializeFrame(frame, buf);/*Deserialize the received segment stored in buf into the created frame*/

                        /*check CRC before reading frame*/

                        if (frame->flags == SYN)/*expected frame received */
                        {
                            printf("Received SYN\n");

                            free(frame);
                            free(buf);

                            frame = newFrame(SYN+ACK, seq, crc, data);//create a SYN+ACK frame

                            buf = newBuffer();/*create helpbuffer for serializing*/

                            serializeFrame(frame, buf);/*Serialize the frame into buf*/

                            /*check crc before sending*/

                            printf("Sending SYN + ACK\n");
                            sendto(fd, buf, sizeof(rtp), 0, (struct sockaddr *)&remaddr, addrlen);/*send serialized_segment in buf to client*/

                            state = SYN_RECEIVED;/*Move to next state*/

                            break;
                        }/*End of expected frame received*/
                        else/*unexpected packet recieved*/
                        {
                            printf("Expecting SYN: Unexpected frame received. Throw away!\n");
                            /*throw away frame, keep listening*/

                            /*delete the created frame/buf */
                            free(frame);
                            free(buf);
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
                printf("Listening for ACK...\n");

                longTimeOut = 600;/*number of short timeouts that will correspond to a long timeout*/

                /*the for-loop represents a long time out, and one iteration represent a short time out*/
                for(numOfshortTimeouts = 0; numOfshortTimeouts < longTimeOut; numOfshortTimeouts++)
                {
                    /*Set time for short timeouts*/
                    shortTimeout.tv_sec = 0;
                    shortTimeout.tv_usec = 200000;

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
                        printf("Reading... \n");

                        /*received serialize segment */
                        recvfrom(fd, serializedSegment, sizeof(rtp), 0, (struct sockaddr *)&remaddr, &addrlen);

                        /*create empty packet struct, to put received serialized segment in*/
                        frame = newFrame(flags, seq, crc, data);

                        /*create helpbuffer for deserializing*/
                        buf = newBuffer();
                        buf->data = serializedSegment;

                        /*Deserialize the received segment stored in buf into the created UDP packet*/
                        deserializeFrame(frame, buf);

                        /*check CRC before opening packet*/

                        /*expected packet received */
                        if (frame->flags == ACK)
                        {
                            printf("Received ACK!\n");

                            state = ESTABLISHED;

                            free(frame);
                            free(buf);

                            break;
                        }
                        /*unexpected packet received, throw away packet, keep listening */
                        else
                        {
                            printf("Expecting ACK: Unexpected packet received! Trow away!\n");
                            free(frame);
                            free(buf);
                        }
                    }/*End of something to read*/
                }/*End of for-loop*/

                printf("Long timeout! Reset connection setup\n");
                state = CLOSED;

                break;

            }/*End of CASE SYN_RECEIVED*/

            case ESTABLISHED:
            {
                printf("Connection astablished!\n");

                while(1)
                {
                    /*Await client to send something, forevA*/
                    select(1, &read_fd_set, NULL, NULL, NULL);

                    /*received serialize segment*/
                    recvfrom(fd, serializedSegment, sizeof(rtp), 0, (struct sockaddr *)&remaddr, &addrlen);

                    /*create empty frame, to put deserialized segment in*/
                    frame = newFrame(flags, seq, crc, data);

                    /*create helpbuffer for deserializing*/
                    buf = newBuffer();
                    buf->data = serializedSegment;

                    /*Deserialize the received segment stored in buf into the created frame*/
                    deserializeFrame(frame, buf);

                    /*check CRC before opening frame*/

                    printf("MSG received: %c \n", frame->data);

                    free(frame);
                    free(buf);
                }

                break;
            }/*End of CASE ESTABLISHED*/
        }/*End of switch*/
    }/*End of while(1)*/

    return 0;
}
