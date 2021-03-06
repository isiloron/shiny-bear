/*
Data communication Spring 2014
Lab 3 - Reliable Transportation Protocol
Students: sdn08003
          lja08001
*/

#ifndef SERVER_H
#define SERVER_H

#include "protocol_std.h"

/*server states*/
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

/*returnval: next state
function: tears down connection*/
int teardownResponse(int fd, struct timeval* shortTimeout, struct sockaddr_in* sourceAddr, int chanceOfFrameError);
/*read errorpercentage from user*/
int getFrameErrorPercentage();

#endif //SERVER_H
