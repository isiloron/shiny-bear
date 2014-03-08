#include "protocol_std.h"
#include "client.h"

void prepareSocket(int* sock_fd, struct sockaddr_in* sockaddr) {
    printf("Preparing socket... ");
    if((*sock_fd=socket(AF_INET, SOCK_DGRAM, 0))<0){
        perror("Could not create socket.");
        return;
    }

    memset((char*)sockaddr, 0, sizeof(*sockaddr));
    sockaddr->sin_family = AF_INET;
    sockaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr->sin_port = htons(PORT);

    if (bind(*sock_fd, (struct sockaddr*)sockaddr, sizeof(*sockaddr)) < 0) {
        perror("Bind failed");
        return;
    }
    printf("Socket prepared!\n");
}
