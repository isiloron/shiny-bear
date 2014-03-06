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

#define PORT 5555
#define BUFFERSIZE 1024

//flags
#define FIN 1 //001
#define SYN 2 //010
#define ACK 4 //100


typedef struct rtp_struct {
    int flags;
    int id; //connection identifier (client socket id)
    int seq; //sequence number (frame number)
    int crc; //crc error check code
    char* data;
} rtp;

rtp* createNewPacket(int flag, int id, int seq, int crc, char* data);

#endif //PROTOCOL_STD_H
