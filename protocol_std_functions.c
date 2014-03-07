#include "protocol_std.h"

rtp* createNewPacket(int flag, int id, int seq, int crc, char data)
{
    rtp* UDP_packet = malloc(sizeof(rtp));
    if(UDP_packet!=NULL)
    {
        perror("Failed to create UDP packet!\n");
        return NULL;
    }
    else
        return UDP_packet;
        //set struct members
}


struct Buffer* newBuffer() {
    #define INITIAL_SIZE 32
    struct Buffer* buf = malloc(sizeof(struct Buffer));

    buf->data = malloc(INITIAL_SIZE);
    buf->size = INITIAL_SIZE;
    buf->next = 0;

    return buf;
}

void reserveSpace(struct Buffer* buf, size_t bytes) {
    if((buf->next + bytes) > buf->size) {
        /* double size to enforce O(lg N) reallocs */
        buf->data = realloc(buf->data, buf->size * 2);
        buf->size *= 2;
    }
}

void serializeRtpStruct(rtp* packet, struct Buffer* buf) {
    serializeInt(packet->crc,buf);
    serializeInt(packet->flags,buf);
    serializeInt(packet->seq,buf);
    serializeChar(packet->data,buf);
}

void serializeInt(int n, struct Buffer* buf) {
    reserveSpace(buf,sizeof(int));
    memcpy( ((char *)buf->data)+(buf->next), &n, sizeof(int));
    buf->next += sizeof(int);
}

void serializeChar(char c, struct Buffer* buf) {
    reserveSpace(buf,sizeof(char));
    memcpy(((char*)buf->data)+(buf->next),&c, sizeof(char));
    buf->next += sizeof(char);
}

void deserializeRtpStruct(rtp* packet, struct Buffer* buf) {
    deserializeInt(&(packet->crc),buf);
    deserializeInt(&(packet->flags),buf);
    deserializeInt(&(packet->seq),buf);
    deserializeChar(&(packet->data),buf);
}

void deserializeInt(int* n, struct Buffer* buf) {
    memcpy(n,((char*)buf->data)+(buf->next), sizeof(int));
    buf->next += sizeof(int);
}

void deserializeChar(char* c, struct Buffer* buf) {
    memcpy(c,((char*)buf->data)+(buf->next), sizeof(char));
    buf->next += sizeof(char);
}





