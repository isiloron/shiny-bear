#include "protocol_std.h"
#include "client.h"


void prepareHostAddr(struct sockaddr_in* servAddr, char* hostName, int port){
    printf("Preparing Host Address... ");
    struct hostent* hostInfo;
    memset((char*)servAddr,0,sizeof(*servAddr));
    servAddr->sin_family = AF_INET;
    servAddr->sin_port = htons(port);
    hostInfo = gethostbyname(hostName);
    if(hostInfo == NULL){
        fprintf(stderr, "prepareHostAddr - Unknown host %s\n",hostName);
        exit(EXIT_FAILURE);
    }
    servAddr->sin_addr = *(struct in_addr *)hostInfo->h_addr;
    printf("Host Address prepared!\n");
}

void prepareSocket(int* sock_fd, struct sockaddr_in* sockaddr) {
    printf("Preparing socket... ");
    if((*sock_fd=socket(AF_INET, SOCK_DGRAM, 0))<0){
        perror("Could not create socket.");
        exit(EXIT_FAILURE);
    }

    memset((char*)sockaddr, 0, sizeof(*sockaddr));
    sockaddr->sin_family = AF_INET;
    sockaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr->sin_port = htons(PORT);

    if (bind(*sock_fd, (struct sockaddr*)sockaddr, sizeof(*sockaddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket prepared!\n");
}
