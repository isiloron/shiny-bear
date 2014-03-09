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

//rtp* receiveFrame(int fd, struct sockaddr* remaddr)
//{
//    rtp* frame = NULL;/*framepointer*/
//    struct Buffer* buf;/*help struct for serializing and deserializing*/
//    void* serializedSegment;
//    socklen_t addrlen = sizeof(remaddr);/* length of addresses */
//
//    if((serializedSegment = malloc(sizeof(rtp))) == NULL) /*assign memory*/
//    {
//        printf("Failed to alocate memory to serializedSegment!\n");
//        exit(0);
//    }
//
//    recvfrom(fd, serializedSegment, sizeof(rtp), 0, (struct sockaddr*)&remaddr, &addrlen);/*received serialized segment*/
//
//    frame = newFrame(0, 0, 0, 0);/*create empty frame, to put serializedSegment in*/
//
//    buf = newBuffer();/*create helpbuffer for deserializing*/
//
//    buf->data = serializedSegment;/*put serialized segment in buffer*/
//
//    deserializeRtpStruct(frame, buf);/*Deserialize the received segment stored in buf into the created frame*/
//
//    /*check frame CRC before reading frame*/
//
//    free(buf);
//
//    return frame;
//}
