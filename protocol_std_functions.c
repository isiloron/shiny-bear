#include "protocol_std.h"

void prepareSocket(int* sock_fd, struct sockaddr_in* sockaddr)
{
    printf("Preparing socket... ");
    if((*sock_fd=socket(AF_INET, SOCK_DGRAM, 0))<0)
    {
        perror("Could not create socket.");
        exit(EXIT_FAILURE);
    }

    memset((char*)sockaddr, 0, sizeof(*sockaddr));
    sockaddr->sin_family = AF_INET;
    sockaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr->sin_port = htons(PORT);

    if (bind(*sock_fd, (struct sockaddr*)sockaddr, sizeof(*sockaddr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket prepared!\n");
}

rtp* newFrame(int flags, int seq, int crc, char data)
{
    rtp* frame = malloc(sizeof(rtp));
    if(frame==NULL)
    {
        perror("Failed to create frame!\n");
        return NULL;
    }

    frame->flags = flags;
    frame->seq = seq;
    frame->crc = crc;
    frame->data = data;

    return frame;
}

#define INITIAL_SIZE 32
struct Buffer* newBuffer()
{
    struct Buffer* buf = malloc(sizeof(struct Buffer));

    buf->data = malloc(INITIAL_SIZE);
    buf->size = INITIAL_SIZE;
    buf->next = 0;

    return buf;
}

void reserveSpace(struct Buffer* buf, size_t bytes)
{
    if((buf->next + bytes) > buf->size)
    {
        /* double size to enforce O(lg N) reallocs */
        buf->data = realloc(buf->data, buf->size * 2);
        buf->size *= 2;
    }
}

void serializeFrame(rtp* frame, struct Buffer* buf)
{
    serializeInt(frame->flags,buf);
    serializeInt(frame->seq,buf);
    serializeChar(frame->data,buf);
    serializeInt(frame->crc,buf);
}

void serializeInt(int n, struct Buffer* buf)
{
    reserveSpace(buf,sizeof(int));
    memcpy( ((char *)buf->data)+(buf->next), &n, sizeof(int));
    buf->next += sizeof(int);
}

void serializeChar(char c, struct Buffer* buf)
{
    reserveSpace(buf,sizeof(char));
    memcpy(((char*)buf->data)+(buf->next),&c, sizeof(char));
    buf->next += sizeof(char);
}

void deserializeFrame(rtp* frame, struct Buffer* buf)
{
    deserializeInt(&(frame->flags),buf);
    deserializeInt(&(frame->seq),buf);
    deserializeChar(&(frame->data),buf);
    deserializeInt(&(frame->crc),buf);
}

void deserializeInt(int* n, struct Buffer* buf)
{
    memcpy(n,((char*)buf->data)+(buf->next), sizeof(int));
    buf->next += sizeof(int);
}

void deserializeChar(char* c, struct Buffer* buf)
{
    memcpy(c,((char*)buf->data)+(buf->next), sizeof(char));
    buf->next += sizeof(char);
}

int sendFrame(int socket, rtp* frame, struct sockaddr_in dest)
{
    struct Buffer* buffer = newBuffer();
    int bytesSent = 0;
    //TODO: CRC
    serializeFrame(frame, buffer);
    htonl(buffer);
    bytesSent = sendto(socket, buffer->data, sizeof(*buffer->data), 0, (struct sockaddr*)&dest, sizeof(dest));
    free(buffer);
    return bytesSent;
}

rtp* receiveFrame(int socket, struct sockaddr_in* sourceAddr)
{
    rtp* frame = NULL;/*framepointer*/
    struct Buffer* buffer;/*help struct for serializing and deserializing*/
    socklen_t addrLen = sizeof(*sourceAddr);
    buffer = newBuffer();/*create helpbuffer for deserializing*/
    recvfrom(socket, buffer->data, sizeof(*(buffer->data)), 0, (struct sockaddr*)sourceAddr, &addrLen);/*received serialized segment*/
    frame = newFrame(0, 0, 0, 0);/*create empty frame*/
    ntohl(buffer);
    deserializeFrame(frame, buffer);/*Deserialize the received segment stored in buffer into the created frame*/

    //TODO: CRC

    free(buffer);

    return frame;
}

void resetShortTimeout(struct timeval* shortTimeout)
{
    shortTimeout->tv_sec = 0;
    shortTimeout->tv_usec = 200000; /*200 millisec*/
}

int waitForFrame(int fd, struct timeval* shortTimeout)
{
    fd_set read_fd_set;
    int returnval;

    FD_ZERO(&read_fd_set);/*clear set*/
    FD_SET(fd, &read_fd_set);/*put the fd in the set for reading*/

    returnval = select(fd + 1, &read_fd_set, NULL, NULL, shortTimeout);/*wait for something to be read on the filedescriptor in the set*/

    if(returnval == -1)/*ERROR*/
    {
        perror("Select failed!");
        exit(EXIT_FAILURE);
    }
    return returnval;
}

//TODO CRC
void setCrc(int* crc, rtp* frame)
{

}

void checkCrc(rtp* frame)
{

}

