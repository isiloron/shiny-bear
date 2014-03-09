#ifndef CLIENT_FUNCTIONS_H
#define CLIENT_FUNCTIONS_H

#define hostNameLength 50

void prepareHostAddr(struct sockaddr_in* servAddr, char* hostName, int port);
void prepareSocket(int* sock_fd, struct sockaddr_in* sockaddr);


#endif //CLIENT_FUNCTIONS_H
