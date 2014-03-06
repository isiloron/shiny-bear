#include "protocol_std.h"

rtp* createNewPacket(int flag, int id, int seq, int crc, char* data)
{
    rtp* UDP_packet = malloc(sizeof(rtp));
    if(UDP_packet!=NULL)
    {
        printf("Failed to create UDP packet!\n");
        return NULL;
    }
    else
        return UDP_packet;
        //set struct members
}

