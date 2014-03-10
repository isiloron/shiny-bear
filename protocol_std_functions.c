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

void deserializeFrame(rtp* packet, struct Buffer* buf)
{
    deserializeInt(&(packet->flags),buf);
    deserializeInt(&(packet->seq),buf);
    deserializeChar(&(packet->data),buf);
    deserializeInt(&(packet->crc),buf);
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

#define CLOSED 1
#define ESTABLISHED 4
#define FIN_SENT 5
#define AWAIT_FIN 6
#define SIMULTANEOUS_CLOSE 7
#define SHORT_WAIT 8

int teardownInitiation(int state, int fd, struct timeval* shortTimeout, struct sockaddr_in* sourceAddr)
{
    rtp* receivedFrame = NULL;
    rtp* frameToSend = NULL;
    int numOfShortTimeouts = 0;
    while(1)
    {
        switch(state)
        {
            case ESTABLISHED:
            {
                frameToSend = newFrame(FIN, 0, 0, 0);//create a FIN frame
                sendFrame(fd, frameToSend, *sourceAddr); /*send FIN*/
                printf("FIN sent \n");
                free(frameToSend);
                state = FIN_SENT;
                break;
            }/*End of case ESTABLISHED*/

            case FIN_SENT:
            {
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    resetShortTimeout(shortTimeout);

                    if(waitForFrame(fd, shortTimeout) == 0) // short timeout, Resend FIN
                    {
                        frameToSend = newFrame(FIN, 0, 0, 0);//recreate a FIN frame
                        sendFrame(fd, frameToSend, *sourceAddr); /*resend FIN*/
                        printf("FIN resent \n");
                        free(frameToSend);
                    }
                    else //frame to read
                    {
                        receivedFrame = receiveFrame(fd, sourceAddr);

                        if(receivedFrame->flags == ACK)/*received expected packet ACK*/
                        {
                            printf("ACK received!\n");
                            state = AWAIT_FIN;
                            break;
                        }
                        else if(receivedFrame->flags == FIN) /*received expected packet FIN (simultaneous close)*/
                        {
                            printf("FIN received \n");
                            frameToSend = newFrame(ACK, 0, 0, 0);//create a ACK frame
                            sendFrame(fd, frameToSend, *sourceAddr);
                            printf("ACK SENT \n");
                            free(receivedFrame);
                            state = SIMULTANEOUS_CLOSE;
                            break;
                        }
                        else /*received unexpected packet*/
                        {
                            printf("Unexpected packet received! \n");
                            free(receivedFrame);
                            break;
                        }
                    }
                }/*End of for-loop*/
                if( (state==SIMULTANEOUS_CLOSE) || (state==AWAIT_FIN) )
                {
                    break;
                }

                printf("Long timeout! State CLOSED.\n");
                state = CLOSED;
                return state;

            }/*End of case FIN_SENT*/
            case SIMULTANEOUS_CLOSE:
            {
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    resetShortTimeout(shortTimeout);

                    if(waitForFrame(fd, shortTimeout) == 0) // short timeout, Resend ACK
                    {
                        frameToSend = newFrame(ACK, 0, 0, 0);//recreate a ACK frame
                        sendFrame(fd, frameToSend, *sourceAddr); /*resend ACK*/
                        printf("ACK resent \n");
                        free(frameToSend);
                    }
                    else //frame to read
                    {
                        receivedFrame = receiveFrame(fd, sourceAddr);

                        if(receivedFrame->flags == ACK)/*received expected packet ACK*/
                        {
                            printf("ACK received!\n");
                            state = SHORT_WAIT;
                            free(receivedFrame);
                            break;
                        }
                        else /*received unexpected packet*/
                        {
                            printf("Unexpected packet received! \n");
                            free(receivedFrame);
                            break;
                        }
                    }
                }/*End of for-loop*/
                if( (state==SIMULTANEOUS_CLOSE) || (state==AWAIT_FIN) )
                {
                    break;
                }

                printf("Long timeout! State CLOSED.\n");
                state = CLOSED;
                return state;

            }/*End of case SIMULTANEOUS_CLOSE*/

            case AWAIT_FIN:
            {
                for(numOfShortTimeouts=0; numOfShortTimeouts<longTimeOut; numOfShortTimeouts++)
                {
                    resetShortTimeout(shortTimeout);

                    if(waitForFrame(fd, shortTimeout) == 0){} // short timeout, do nothing
                    else //frame to read
                    {
                        receivedFrame = receiveFrame(fd, sourceAddr);

                        if(receivedFrame->flags == FIN)/*received expected packet FIN*/
                        {
                            printf("FIN received!\n");
                            state = SHORT_WAIT;
                            free(receivedFrame);
                            break;
                        }
                        else /*received unexpected packet*/
                        {
                            printf("Unexpected packet received! \n");
                            free(receivedFrame);
                            break;
                        }
                    }
                }/*End of for-loop*/
                if(state==SHORT_WAIT)
                {
                    break;
                }

                printf("Long timeout! State CLOSED.\n");
                state = CLOSED;
                return state;

            }/*End of case AWAIT_FIN*/

            case SHORT_WAIT:
            {
                resetShortTimeout(shortTimeout);

                if(waitForFrame(fd, shortTimeout) == 0)// short timeout, socket closed successfully
                {
                    printf("Long timeout! State CLOSED.\n");
                    state = CLOSED;
                    return state;
                }
            }/*End of case SHORT_WAIT*/
        }/*End of switch*/
    }/*End of while*/
}

//TODO CRC
void setCrc(int* crc, rtp* frame)
{

}

void checkCrc(rtp* frame)
{

}

