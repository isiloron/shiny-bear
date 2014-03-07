#include "protocol_std.h"

rtp* newFrame(int flags, int seq, int crc, char data)
{
    rtp* frame = malloc(sizeof(rtp));
    if(frame!=NULL)
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
struct Buffer* newBuffer() {
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

void serializeRtpStruct(rtp* frame, struct Buffer* buf) {
    serializeInt(frame->flags,buf);
    serializeInt(frame->seq,buf);
    serializeChar(frame->data,buf);
    serializeInt(frame->crc,buf);
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
    deserializeInt(&(packet->flags),buf);
    deserializeInt(&(packet->seq),buf);
    deserializeChar(&(packet->data),buf);
    deserializeInt(&(packet->crc),buf);
}

void deserializeInt(int* n, struct Buffer* buf) {
    memcpy(n,((char*)buf->data)+(buf->next), sizeof(int));
    buf->next += sizeof(int);
}

void deserializeChar(char* c, struct Buffer* buf) {
    memcpy(c,((char*)buf->data)+(buf->next), sizeof(char));
    buf->next += sizeof(char);
}


//TODO CRC
void setCrc(int* crc, rtp* frame) {

}

void checkCrc(rtp* frame) {

}

