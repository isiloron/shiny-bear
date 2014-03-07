#include "server.h"

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
    int id = 0;
    int seq = 0;
    int crc = 0;
    char data = 0;

    /*Timeouts*/
    int longTimeOut = 0;
    rtp* UDP_packet = NULL;
    struct timeval shortTimeOut;
    int numOfShortTimeOuts = 0; /*iteration konstant for tiemouts*/

    while(1)
    {
        switch(state)
        {
            case CLOSED:
            {
                /*Close (if any) old socket*/
                close(fd);

                /* Try to create and open up a new UDP socket */
                if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                {
                    printf("Could not create UDP socket!\n");
                    exit(0);
                }
                /* Try to bind the socket to any valid IP address*/
                else
                {
                    printf("Successfully created a Socket!\n");

                    memset((char *)&myaddr, 0, sizeof(myaddr));
                    myaddr.sin_family = AF_INET;
                    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
                    myaddr.sin_port = htons(PORT);

                    if(bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
                    {
                        printf("Could not bind the socket!\n");
                        exit(0);
                    }
                    else/*created and bound a socket successfully, go to next state*/
                    {
                        printf("Successfully bound a Socket!\n");
                        state = LISTEN;
                    }
                }
                break;

            }/*End of case CLOSED*/

            case LISTEN:
            {
                    /*wait for SYN from client*/
                     printf("Listening for SYN...\n");

                    longTimeOut = 600;/*number of short timeouts that will correspond to a long timeout*/
                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set*/

                    /*the for-loop represents a long time out, and one iteration represent a short time out*/
                    for(numOfShortTimeOuts = 0; numOfShortTimeOuts < longTimeOut; numOfShortTimeOuts++)
                    {
                        /*Set time for short timeouts*/
                        shortTimeOut.tv_sec = 0;
                        shortTimeOut.tv_usec = 200000;

                        /*wait for something to be read on the filedescriptor in the set*/
                        returnval = select(1, &read_fd_set, NULL, NULL, &shortTimeOut);

                        if(returnval == -1)/*ERROR*/
                        {
                            printf("Select failed!\n");
                            exit(0);
                        }
                        else if(returnval == 0){}/*short timeout, no request to connect received*/
                        else/*there is something to read on the filedescriptor*/
                            break;
                    }

                    if(returnval == 1)/*we got something to read*/
                    {
                        /*received serialized segment*/
                        recvfrom(fd, serializedSegment, sizeof(rtp), 0, (struct sockaddr *)&remaddr, &addrlen);

                        /*create empty packet struct, to put serializedSegment in*/
                        UDP_packet = createNewPacket(flags, id, seq, crc, data);

                        /*create helpbuffer for deserializing*/
                        buf = newBuffer();
                        buf->data = serializedSegment;

                        /*Deserialize the received segment stored in buf into the created UDP packet*/
                        deserializeRtpStruct(UDP_packet, buf);

                        /*check CRC before opening packet*/

                        /*expected packet received */
                        if (UDP_packet->flags == SYN)
                        {
                            printf("Received SYN!\n");

                            free(UDP_packet);
                            free(buf);

                            //create a SYN+ACK packet
                            UDP_packet = createNewPacket(flags, id, seq, crc, data);

                            /*create helpbuffer for serializing*/
                            buf = newBuffer();

                            /*Serialize the packet into buf*/
                            serializeRtpStruct(UDP_packet, buf);

                            /*check crc before sending*/

                            /*send serialized_segment in buf to client*/
                            sendto(fd, buf, sizeof(rtp), 0, (struct sockaddr *)&remaddr, addrlen);

                            /*Move to next state*/
                            state = SYN_RECEIVED;


                        }
                        else/*unexpected packet recieved*/
                        {
                            printf("unexpected packet! Trow away!\n");
                            /*throw away packet, keep listening*/
                            state = LISTEN;
                        }

                        /*delete the created packet/buf */
                        free(UDP_packet);
                        free(buf);
                    }
                    else/*Long time out triggered! Reset connection setup*/
                    {
                        printf("Long timeout!\n");
                        state = CLOSED;
                    }

                break;

            }/*End of case LISTEN*/

            case SYN_RECEIVED:
            {
                    /*Awaiting ACK from client*/

                    printf("Listening for ACK...\n");

                    longTimeOut = 600;/*number of short timeouts that will correspond to a long timeout*/
                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set*/

                    /*the for-loop represents a long time out, and one iteration represent a short time out*/
                    for(numOfShortTimeOuts = 0; numOfShortTimeOuts < longTimeOut; numOfShortTimeOuts++)
                    {
                        /*Set time for short timeouts*/
                        shortTimeOut.tv_sec = 0;
                        shortTimeOut.tv_usec = 200000;

                        /*wait for something to be read on the filedescriptor in the set*/
                        returnval = select(1, &read_fd_set, NULL, NULL, &shortTimeOut);

                        if(returnval == -1)/*ERROR*/
                        {
                            perror("Select failed!\n");
                            exit(0);
                        }
                        else if(returnval == 0){}/*short timeout, no request to connect received*/
                        else/*there is something to read on the filedescriptor*/
                            break;
                    }

                    if(returnval == 1)/*we got something to read*/
                    {
                        /*received serialize segment */
                        recvfrom(fd, serializedSegment, sizeof(rtp), 0, (struct sockaddr *)&remaddr, &addrlen);

                        /*create empty packet struct, to put received serialized segment in*/
                        UDP_packet = createNewPacket(flags, id, seq, crc, data);

                        /*create helpbuffer for deserializing*/
                        buf = newBuffer();
                        buf->data = serializedSegment;

                        /*Deserialize the received segment stored in buf into the created UDP packet*/
                        deserializeRtpStruct(UDP_packet, buf);

                        /*check CRC before opening packet*/

                        /*expected packet received */
                        if (UDP_packet->flags == ACK)
                        {
                            printf("Received ACK!\n");

                            state = ESTABLISHED;

                            free(UDP_packet);
                            free(buf);

                            break;
                        }
                        /*unexpected packet received, throw away packet, keep listening */
                        else
                        {
                            free(UDP_packet);
                            free(buf);
                            state = LISTEN;
                        }
                    }
                    else/*Long time out triggered! Reset connection setup*/
                    {
                        printf("Long timeout!\n");
                        state = CLOSED;
                    }

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

                    /*create empty UDP_packet, to put deserialized segment in*/
                    UDP_packet = createNewPacket(flags, id, seq, crc, data);

                    /*create helpbuffer for deserializing*/
                    buf = newBuffer();
                    buf->data = serializedSegment;

                    /*Deserialize the received segment stored in buf into the created UDP packet*/
                    deserializeRtpStruct(UDP_packet, buf);

                    /*check CRC before opening packet*/

                    printf("MSG received: %c \n", UDP_packet->data);

                    free(UDP_packet);
                    free(buf);
                }
                break;

            }/*End of CASE ESTABLISHED*/

        }/*End of switch*/
    }

    return 0;
}
