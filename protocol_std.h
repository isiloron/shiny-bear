#ifndef PROTOCOL_STD_H
#define PROTOCOL_STD_H

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>

#define PORT 5555
#define MAXMSG 254 //char (-1 for '\0')
#define MAXSEQ 16
#define WINDSIZE 8

//flags
#define FIN 1 //0001
#define SYN 2 //0010
#define ACK 4 //0100
#define INF 8 //1000

/*Timeouts*/
#define longTimeOut 600

typedef struct rtp_struct
{
    int flags;
    int seq; //sequence number (frame number)
    int crc; //crc error check code
    char data; //one character at a time, if flag is INF then the char value is the number of characters in the complete message.
} rtp;

struct Buffer
{
    void* data;
    int next;
    size_t size;
};

void prepareSocket(int* sock_fd, struct sockaddr_in* sockaddr);
rtp* newFrame(int flags, int seq, int crc, char data);
struct Buffer* newBuffer();
void reserveSpace(struct Buffer* buf, size_t bytes);
void serializeFrame(rtp* frame, struct Buffer* buf);
void serializeInt(int n, struct Buffer* buf);
void serializeChar(char c, struct Buffer* buf);
void deserializeFrame(rtp* packet, struct Buffer* buf);
void deserializeInt(int* n, struct Buffer* buf);
void deserializeChar(char* c, struct Buffer* buf);
int sendFrame(int socket, rtp* frame, struct sockaddr_in dest);
rtp* receiveFrame(int socket, struct sockaddr_in* sourceAddr);
void resetShortTimeout(struct timeval* shortTimeout);
int waitForFrame(int fd, struct timeval* shortTimeout);
int teardownInitiation(int fd, struct timeval* shortTimeout, struct sockaddr_in* sourceAddr);



#endif //PROTOCOL_STD_H
