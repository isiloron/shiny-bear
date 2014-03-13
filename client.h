#ifndef CLIENT_FUNCTIONS_H
#define CLIENT_FUNCTIONS_H

#define hostNameLength 50

struct windowStruct
{
    int sfd;
    struct sockaddr_in servAddr;
    rtp* frameSeq[MAXSEQ];
    int startSeq;
    int endSeq; // is one greater than the last seq sent, equal to startSeq if no frames has been sent, endSeq-startSeq = number of frames sent
    int errorChance;
};

void prepareHostAddr(struct sockaddr_in* servAddr, char* hostName, int port);
int clientSlidingWindow(int sfd, struct sockaddr_in* servAddr, int errorChance);
void *inputThreadFunction(void *arg);
void resendFrames(struct windowStruct *window);


#endif //CLIENT_FUNCTIONS_H
