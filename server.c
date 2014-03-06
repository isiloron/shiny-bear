#include "server.h"

int main(int argc, char **argv)
{
    int state = CLOSED;
    struct sockaddr_in myaddr;/* our address */
    struct sockaddr_in remaddr;/* remote address */
    socklen_t addrlen = sizeof(remaddr);/* length of addresses */
    int recvlen;/* bytes received */
    int fd;/* our socket (filedescriptor)*/
    char buffer[BUFFERSIZE];/* receive buffer */

    switch(state)
    {
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
            while(1)
            {
                /*Awaiting SYN from client*/
                recvlen = recvfrom(fd, buffer, BUFFERSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);

                /*SYN received, send SYN+ACK*/
                if (recvlen > 0)
                {
                    buffer[recvlen] = 0;
                    printf("Received SYN: \"%s\"\n", buffer);

                    //create a packet to send

                    sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr *)&remaddr, addrlen);
                    state = SYN_RECEIVED;
                    break;
                }
            }

            break;
        }/*End of case LISTEN*/

        case SYN_RECEIVED:
        {
            /*Awaiting ACK from client*/

            recvlen = recvfrom(fd, buffer, BUFFERSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);

                if (recvlen > 0)
                {
                    buffer[recvlen] = 0;
                    printf("Received ACK: \"%s\"\n", buffer);
                    state = ESTABLISHED;
                    break;
                }

            break;
        }/*End of CASE SYN_RECEIVED*/

        case ESTABLISHED:
        {
            /*Await client to send frames*/

            break;
        }/*End of CASE ESTABLISHED*/

    }/*End of switch*/

    return 0;
}
