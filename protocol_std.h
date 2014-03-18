/*
Data communication Spring 2014
Lab 3 - Reliable Transportation Protocol
Students: sdn08003
          lja08001
*/

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
#define MAXMSG 255 // 255 character message with a \0 character at the end

//Frame
#define BUFFERSIZE 4 //size in bytes of buffer to send over UDP 1 byte each for flags, seq, data and crc.

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
    uint8_t flags; // type of frame
    uint8_t seq; //sequence number (frame number)
    char data; //one character at a time, if flag is INF then the char value is the number of characters in the complete message.
} rtp;

struct Buffer
{
    void* data; //the actual data of the buffer
    int next; //number of bytes to the next chunk of memory to read or write
    size_t size; //the size of the buffer, in this program is will always be equal to BUFFERSIZE
};

//Socket functions
void prepareSocket(int* sock_fd, struct sockaddr_in* sockaddr);

//Frames
rtp* newFrame(int flags, int seq, char data);

//serializing
struct Buffer* newBuffer();
void serializeFrame(rtp* frame, struct Buffer* buf);
void serialize_uint8(uint8_t n, struct Buffer* buf);
void serialize_char(char c, struct Buffer* buf);
void deserializeFrame(rtp* frame, struct Buffer* buf);
void deserialize_uint8(uint8_t* n, struct Buffer* buf);
void deserialize_char(char* c, struct Buffer* buf);

//sending and receiving frames
void sendFrame(int socket, rtp* frame, struct sockaddr_in dest, int chanceOfFrameError);
rtp* receiveFrame(int socket, struct sockaddr_in* sourceAddr);
void resetShortTimeout(struct timeval* shortTimeout);
int waitForFrame(int fd, struct timeval* shortTimeout);

//error generation
int generateError(int chanceOfFrameError);

//CRC
#define CRCPOLY 0xD5 //CRC-8
#define TOPBIT 128 //binary 10000000
uint8_t crcTable[256]; //global declaration
void initCrc();
uint8_t getCrc(uint8_t *buffer);
void setCrc(uint8_t *buffer);
bool checkCrc(uint8_t *buffer);

#endif //PROTOCOL_STD_H
