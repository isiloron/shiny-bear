#ifndef SERVER_H
#define SERVER_H

#include "protocol_std.h"

/*states on server*/
#define CLOSED 1
#define LISTEN 2
#define SYN_RECEIVED 3
#define ESTABLISHED 4

int createSocket();
int bind_UDP_Socket(struct sockaddr_in* myaddr, int fd);
void resetShortTimeout(struct timeval* shortTimeout);
//rtp* receiveFrame(int fd, struct sockaddr* remaddr);

#endif //SERVER_H
