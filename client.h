#ifndef CLIENT_FUNCTIONS_H
#define CLIENT_FUNCTIONS_H

#define hostNameLength 50

void prepareHostAddr(struct sockaddr_in* servAddr, char* hostName, int port);
int clientSlidingWindow(int sfd, struct sockaddr_in* servAddr);


#endif //CLIENT_FUNCTIONS_H
