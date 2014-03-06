#include "protocol_std.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h> /* for fprintf */
#include <string.h> /* for memcpy */
#include <netinet/in.h>

#define PORT 555
#define BUFSIZE 2048

/*Content of UDP packets*/
typedef struct UDP_packet_struct
{
    int flags;          /*8 bit sequence determing type of packet*/
    int id;             /*client id*/
    int seq;            /*number of current frame (expected frame)*/
    int windowsize;     /*???*/
    int crc;            /*crc checksum*/
    char *data;         /*part of total buffer (4 bytes/packet)*/
}UDP_packet;
