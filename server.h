#ifndef SERVER_H
#define SERVER_H

#include "protocol_std.h"


/*states on server*/
#define CLOSED 1
#define LISTEN 2
#define SYN_RECEIVED 3
#define ESTABLISHED 4
#define RESPOND_TEARDOWN 5
#define FIN_SENT 5
#define AWAIT_FIN 6
#define SIMULTANEOUS_CLOSE 7
#define SHORT_WAIT 8
#define AWAIT_CLOSE 9
#define AWAIT_ACK 10

int teardownResponse(int fd, struct timeval* shortTimeout, struct sockaddr_in* sourceAddr, int chanceOfFrameError);


#endif //SERVER_H
