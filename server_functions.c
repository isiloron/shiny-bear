#include "server.h"

int createSocket()
{
    int UDP_filedescriptor;

    if ((UDP_filedescriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("Could not create UDP socket!\n");
        exit(0);
    }
    else
    {
        printf("Successfully created a Socket!\n");
        return UDP_filedescriptor;
    }
}

int bind_UDP_Socket(struct sockaddr_in* myaddr, int fd)
{
    /* Try to bind the socket to any valid IP address*/
    memset((char *)myaddr, 0, sizeof(*myaddr)); /*assign memory to my address*/
    myaddr->sin_family = AF_INET;/*assign protocolfamily */
    myaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr->sin_port = htons(PORT);/*assign port*/

    if(bind(fd, (struct sockaddr *)myaddr, sizeof(*myaddr)) < 0)
    {
        printf("Could not bind the socket!\n");
        exit(0);
    }
    else/*created and bound a socket successfully, go to next state*/
    {
        printf("Successfully bound a Socket!\n");
        return LISTEN;
    }
}

void resetShortTimeout(struct timeval* shortTimeout)
{
    shortTimeout->tv_sec = 0;
    shortTimeout->tv_usec = 200000; /*200 millisec*/
}
