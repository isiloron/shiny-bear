#include "server.h"

int main(int argc, char **argv)
{
    int state = CLOSED;
    struct sockaddr_in myaddr;/* our address */
    struct sockaddr_in remaddr;/* remote address */
    socklen_t addrlen = sizeof(remaddr);/* length of addresses */
    int fd;/* our socket (filedescriptor)*/
    char buffer[BUFFERSIZE];/* Buffer containing the readable data*/
    void* packet_buffer;/* Buffer containing packets*/
    int recvlen;
    int i = 0; /*iteration konstant for tiemouts*/

    int flags = 0;
    int id = 0;
    int seq = 0;
    int crc = 0;
    char* data = NULL;
    int longTimeOut;
    rtp* outgoing_packet = NULL;
    rtp* incoming_packet = NULL;
    fd_set read_fd_set; /*create a set for sockets*/

    struct timeval shortTimeOut;

    int returnval; /*for checking returnval on select()*/

    while(1)
    {
        switch(state)
        {
            case CLOSED:
            {
                /*Close the old socket*/
                close(fd);

                /* Try to create and open up a new UDP socket */
                if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                {
                    perror("Could not create UDP socket!\n");
                    exit(0);
                }
                /* Try to bind the socket to any valid IP address*/
                else
                {
                    memset((char *)&myaddr, 0, sizeof(myaddr));
                    myaddr.sin_family = AF_INET;
                    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
                    myaddr.sin_port = htons(PORT);

                    if(bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
                    {
                        perror("Could not bind the socket!\n");
                        exit(0);
                    }
                    else/*created and bound a socket successfully, go to next state*/
                        state = LISTEN;
                }
                break;

            }/*End of case CLOSED*/

            case LISTEN:
            {
                    /*Awaiting SYN from client*/
                    longTimeOut = 5;/*number of short timeouts that will correspond to a long timeout*/
                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set*/

                    /*the for-loop represents a long time out, and one iteration represent a short time out*/
                    for(i = 0; i < longTimeOut; i++)
                    {
                        /*Set time for short timeouts*/
                        shortTimeOut.tv_sec = 5;
                        shortTimeOut.tv_usec = 0;

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
                        /*received packet is stored in packetbuffer (void* packetbuffer)*/
                        recvlen = recvfrom(fd, packet_buffer, sizeof(rtp), 0, (struct sockaddr *)&remaddr, &addrlen);

                        /*create empty packet struct, to received packetbuffer*/
                        incoming_packet = createNewPacket(flags, id, seq, crc, data);

                        /*typecast packetbuffer to packet struct ptr */
                        incoming_packet = (rtp*)packet_buffer;

                        /*Make sure packet is ok before "opening" the packet? built into UDP protocol?*/

                        /*expected packet received */
                        if (incoming_packet->flags == SYN)
                        {
                            printf("Received SYN!\n");

                            //create a SYN+ACK packet
                            outgoing_packet = createNewPacket(flags, id, seq, crc, data);

                            /*put packet in packet_buffer*/
                            packet_buffer = (void*)outgoing_packet;

                            /*send packet_buffer to client*/
                            sendto(fd, packet_buffer, sizeof(rtp), 0, (struct sockaddr *)&remaddr, addrlen);

                            /*Move to next state*/
                            state = SYN_RECEIVED;

                            /*delete the created packet*/
                            free(outgoing_packet);
                        }
                        else/*unexpected packet recieved*/
                        {
                            /*throw away packet, keep listening*/
                            free(incoming_packet);
                            state = LISTEN;
                        }
                    }
                    else/*Long time out triggered! Reset connection setup*/
                    {
                        state = CLOSED;
                    }

                break;

            }/*End of case LISTEN*/

            case SYN_RECEIVED:
            {
                    /*Awaiting ACK from client*/
                    longTimeOut = 5;/*number of short timeouts that will correspond to a long timeout*/
                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set*/

                    /*the for-loop represents a long time out, and one iteration represent a short time out*/
                    for(i = 0; i < longTimeOut; i++)
                    {
                        /*Set time for short timeouts*/
                        shortTimeOut.tv_sec = 5;
                        shortTimeOut.tv_usec = 0;

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
                        /*received packet is stored in packetbuffer (void* packetbuffer)*/
                        recvlen = recvfrom(fd, packet_buffer, sizeof(rtp), 0, (struct sockaddr *)&remaddr, &addrlen);

                        /*create empty packet struct, to received packetbuffer*/
                        incoming_packet = createNewPacket(flags, id, seq, crc, data);

                        /*typecast packetbuffer to packet struct ptr */
                        incoming_packet = (rtp*)packet_buffer;

                        /*Make sure packet is ok before "opening" the packet? built into UDP protocol?*/

                        /*expected packet received */
                        if (incoming_packet->flags == ACK)
                        {
                            printf("Received ACK!\n");
                            state = ESTABLISHED;
                            break;
                        }
                        /*unexpected packet received, throw away packet, keep listening */
                        else
                        {
                            free(incoming_packet);
                            state = LISTEN;
                        }
                    }
                    else/*Long time out triggered! Reset connection setup*/
                    {
                        state = CLOSED;
                    }

                break;

            }/*End of CASE SYN_RECEIVED*/

            case ESTABLISHED:
            {
                while(1)
                {
                    /*Await client to send something, for evA*/
                    select(1, &read_fd_set, NULL, NULL, NULL);

                    /*received packet is stored in packetbuffer (void* packetbuffer)*/
                    recvlen = recvfrom(fd, packet_buffer, sizeof(rtp), 0, (struct sockaddr *)&remaddr, &addrlen);

                    /*create empty packet struct, to received packetbuffer*/
                    incoming_packet = createNewPacket(flags, id, seq, crc, data);

                    /*typecast packetbuffer to packet struct ptr */
                    incoming_packet = (rtp*)packet_buffer;

                    /*read data from packet*/
                    printf("Received message: %s \n", incoming_packet->data);

                    /*throw away packet, when it has been read*/
                    free(incoming_packet);
                }
                break;

            }/*End of CASE ESTABLISHED*/

        }/*End of switch*/
    }

    return 0;
}
