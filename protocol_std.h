#ifndef PROTOCOL_STD_H
#define PROTOCOL_STD_H

//Libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

//Socket
#define PORT 5555

//Message
#define MAXMSG 254 //char (-1 for '\0')

//Frame
#define BUFFERSIZE 16 //size of buffer to send over UDP

//Sliding window
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
    char data; //one character at a time, if flag is INF then the char value is the number of characters in the complete message.
} rtp;

struct Buffer
{
    void* data;
    int next;
    size_t size;
};

//Socket functions
void prepareSocket(int* sock_fd, struct sockaddr_in* sockaddr);

//Frames
rtp* newFrame(int flags, int seq, char data);

struct Buffer* newBuffer();
void reserveSpace(struct Buffer* buf, size_t bytes);
void serializeFrame(rtp* frame, struct Buffer* buf);
void serializeInt(int n, struct Buffer* buf);
void serializeChar(char c, struct Buffer* buf);
void deserializeFrame(rtp* frame, struct Buffer* buf);
void deserializeInt(int* n, struct Buffer* buf);
void deserializeChar(char* c, struct Buffer* buf);
int sendFrame(int socket, rtp* frame, struct sockaddr_in dest, int chanceOfFrameError);
rtp* receiveFrame(int socket, struct sockaddr_in* sourceAddr);
void resetShortTimeout(struct timeval* shortTimeout);
int waitForFrame(int fd, struct timeval* shortTimeout);
int getFrameErrorPercentage();
int generateError(int chanceOfFrameError);
void convToBinary(char* data);
void checkCrc(struct Buffer* buffer);


#endif //PROTOCOL_STD_H
