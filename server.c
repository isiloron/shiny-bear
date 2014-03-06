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

    int flag = 0;
    int id = 0;
    int seq = 0;
    int crc = 0;
    char* data = NULL;
    rtp* packetPtr = NULL;
    rtp* incoming_packet = NULL;
    fd_set read_fd_set; /*create a set for sockets*/

    struct timeval shortTimeOut;
    struct timeval longTimeOut;
    int returnval; /*for checking returnval on select()*/

    while(1)
    {
        switch(state)
        {
            /*Reset time outs*/
            shortTimeOut.tv_sec = 5;
            shortTimeOut.tv_usec = 0;
            longTimeOut.tv_sec = 20;
            longTimeOut.tv_usec = 0;

            case CLOSED:
            {
                /* Try to create and open up a UDP socket */
                if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                {
                    perror("Could not create UDP socket!\n");
                    return 0;
                }

                /* Try to bind the socket to any valid IP address and the specified port */
                memset((char *)&myaddr, 0, sizeof(myaddr));
                myaddr.sin_family = AF_INET;
                myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
                myaddr.sin_port = htons(PORT);

                if(bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
                {
                    perror("Could not bind the socket!\n");
                    return 0;
                }

                state = LISTEN;
                break;

            }/*End of case CLOSED*/

            case LISTEN:
            {
                    /*Awaiting SYN from client with select, TODO! MAKE FORLOOP TO SIMULATE LONG TIMEOUT FOR ALL STATES*/

                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set*/

                    returnval = select(1, &read_fd_set, NULL, NULL, &shortTimeOut); /*wait for something to be read on the fd in the set*/

                    if(returnval == 0)/*short time out, noone wants to connect*/
                        break;
                    else if(returnval == -1)/*ERROR*/
                    {
                        perror("Select() failed!\n");
                        exit(0);
                    }
                    else/*we got something to read*/
                    {
                        /*store received packet in packetbuffer*/
                        recvlen = recvfrom(fd, packet_buffer, sizeof(rtp), 0, (struct sockaddr *)&remaddr, &addrlen);

                        /*Bytecount? */
                        if(recvlen > 0)
                        {
                            incoming_packet = (rtp*)packet_buffer; /*typecast void* to struct* */

                            /*Is this SYN? */
                            if (incoming_packet->flags == SYN)
                            {
                                printf("Received SYN!\n");

                                //create a packet to send
                                packetPtr = createNewPacket(flag, id, seq, crc, data);

                                sendto(fd, packet_buffer, sizeof(rtp), 0, (struct sockaddr *)&remaddr, addrlen);
                                state = SYN_RECEIVED;
                                free(packetPtr);
                            }
                        }
                    }

                break;

            }/*End of case LISTEN*/

            case SYN_RECEIVED:
            {
                    /*Awaiting ACK from client with select*/

                    FD_ZERO(&read_fd_set);/*clear set*/
                    FD_SET(fd,&read_fd_set);/*put the fd in the set*/

                    returnval = select(1, &read_fd_set, NULL, NULL, &shortTimeOut); /*wait for something to be read on the fd in the set*/

                    if(returnval == 0)/*short time out, ACK has been lost!*/
                        break;
                    else if(returnval == -1)/*ERROR*/
                    {
                        perror("Select() failed!\n");
                        exit(0);
                    }
                    else/*we got something to read*/
                    {
                        recvlen = recvfrom(fd, packet_buffer, sizeof(rtp), 0, (struct sockaddr *)&remaddr, &addrlen);

                        /*Have we received any bytes? */
                        if(recvlen > 0)
                        {
                            incoming_packet = (rtp*)packet_buffer; /*typecast void* to struct* */

                            /*Is this ACK?*/
                            if (incoming_packet->flags == ACK)
                            {
                                printf("Received ACK!\n");
                                state = ESTABLISHED;
                                break;
                            }
                        }
                    }

                break;

            }/*End of CASE SYN_RECEIVED*/

            case ESTABLISHED:
            {
                /*Await client to send frames*/
                break;

            }/*End of CASE ESTABLISHED*/

        }/*End of switch*/
    }

    return 0;
}
