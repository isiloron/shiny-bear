#include "server.h"

int main(int argc, char **argv)
{
    struct sockaddr_in myaddr;/* our address */
    struct sockaddr_in remaddr;/* remote address */
    socklen_t addrlen = sizeof(remaddr);/* length of addresses */
    int recvlen;/* bytes received */
    int fd;/* our socket (filedescriptor)*/
    unsigned char buf[BUFFERSIZE];/* receive buffer */

    /* create a UDP socket */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Could not create UDP socket!\n");
        return 0;
    }

    /* bind the socket to any valid IP address and a specific port */
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(PORT);

    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
    {
        perror("Could not bind the socket!\n"); return 0;
    }

 /* now loop, receiving data and printing what we received */

    while(1)
    {
        printf("Listening on port %d\n", PORT);
        recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
        printf("Received %d bytes\n", recvlen);

        if (recvlen > 0)
        {
            buf[recvlen] = 0;
            printf("Received message: \"%s\"\n", buf);
        }
    } /* never exits */
}
