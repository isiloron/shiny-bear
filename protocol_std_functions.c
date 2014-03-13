#include "protocol_std.h"

void prepareSocket(int* sock_fd, struct sockaddr_in* sockaddr)
{
    if((*sock_fd=socket(AF_INET, SOCK_DGRAM, 0))<0)
    {
        perror("Could not create socket.");
        exit(EXIT_FAILURE);
    }

    memset((char*)sockaddr, 0, sizeof(*sockaddr));
    sockaddr->sin_family = AF_INET;
    sockaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr->sin_port = htons(PORT);

    if (bind(*sock_fd, (struct sockaddr*)sockaddr, sizeof(*sockaddr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket prepared \n");
}

rtp* newFrame(int flags, int seq, char data)
{
    rtp* frame = malloc(sizeof(rtp));
    if(frame==NULL)
    {
        perror("Failed to create frame \n");
        return NULL;
    }

    frame->flags = flags;
    frame->seq = seq;
    frame->data = data;

    return frame;
}

struct Buffer* newBuffer()
{
    struct Buffer* buf = malloc(sizeof(struct Buffer));

    buf->data = malloc(BUFFERSIZE);
    buf->size = BUFFERSIZE;
    buf->next = 0;

    return buf;
}

void serializeFrame(rtp* frame, struct Buffer* buf)
{
    serialize_int(frame->flags,buf);
    serialize_int(frame->seq,buf);
    serialize_char(frame->data,buf);
}

void serialize_int(int n, struct Buffer* buf)
{
    memcpy( ((char *)buf->data)+(buf->next), &n, sizeof(int));
    buf->next += sizeof(int);
}

void serialize_char(char c, struct Buffer* buf)
{
    memcpy(((char*)buf->data)+(buf->next),&c, sizeof(char));
    buf->next += sizeof(char);
}

void deserializeFrame(rtp* frame, struct Buffer* buf)
{
    deserialize_int(&(frame->flags),buf);
    deserialize_int(&(frame->seq),buf);
    deserialize_char(&(frame->data),buf);
}

void deserialize_int(int* n, struct Buffer* buf)
{
    memcpy(n,((char*)buf->data)+(buf->next), sizeof(int));
    buf->next += sizeof(int);
}

void deserialize_char(char* c, struct Buffer* buf)
{
    memcpy(c,((char*)buf->data)+(buf->next), sizeof(char));
    buf->next += sizeof(char);
}

int sendFrame(int socket, rtp* frame, struct sockaddr_in dest, int chanceOfFrameError)
{
    /*
    if(generateError(chanceOfFrameError) == 1)
    {
        printf("Frame dissapear \n");
    }
    else if(generateError(chanceOfFrameError) == 2)
    {
        printf("CRC error \n");
    }
    else
    {
        printf("Frame ok \n");
    }
    */

    struct Buffer* buffer = newBuffer();
    int bytesSent = 0;
    //TODO: CRC
    serializeFrame(frame, buffer);

    bytesSent = sendto(socket, buffer->data, buffer->size, 0, (struct sockaddr*)&dest, sizeof(dest));
    free(buffer);
    return bytesSent;
}

rtp* receiveFrame(int socket, struct sockaddr_in* sourceAddr)
{
    rtp* frame = NULL;/*framepointer*/
    struct Buffer* buffer;/*help struct for serializing and deserializing*/
    socklen_t addrLen = sizeof(*sourceAddr);
    buffer = newBuffer();/*create helpbuffer for deserializing*/
    recvfrom(socket, buffer->data, buffer->size, 0, (struct sockaddr*)sourceAddr, &addrLen);/*received serialized segment*/
    frame = newFrame(0, 0, 0);/*create empty frame*/

    //checkCrc(buffer);

    deserializeFrame(frame, buffer);/*Deserialize the received segment stored in buffer into the created frame*/

    free(buffer);

    return frame;
}

void resetShortTimeout(struct timeval* shortTimeout)
{
    shortTimeout->tv_sec = 0;
    shortTimeout->tv_usec = 200000; /*200 millisec*/
}

int waitForFrame(int fd, struct timeval* shortTimeout)
{
    fd_set read_fd_set;
    int returnval;

    FD_ZERO(&read_fd_set);/*clear set*/
    FD_SET(fd, &read_fd_set);/*put the fd in the set for reading*/

    returnval = select(fd + 1, &read_fd_set, NULL, NULL, shortTimeout);/*wait for something to be read on the filedescriptor in the set*/

    if(returnval == -1)/*ERROR*/
    {
        perror("Select failed!");
        exit(EXIT_FAILURE);
    }
    return returnval;
}

//TODO CRC
void setCrc(int* crc, rtp* frame)
{


}

void checkCrc(struct Buffer* buffer)
{
    buffer->data = (char*)buffer->data;
    convToBinary(buffer->data);
}

void convToBinary(char* data)/*rewrite int as binary*/
{
   char binaryArr[31];
   int i = 0;

   printf("Binary representation of %d (int) \n", (int)*data);

   while(*data > 0)
   {
       binaryArr[i] = (int)*data % 2; /*rest of data, when div with 2*/
       *data= (*data/2);/*update data*/
       i++;
   }
   while(i < 31)
   {
       binaryArr[i] = 0;
       i++;
   }

    /*print in reverse order*/

    printf("This is the binary representation of the buffer: \n");

    for(i = 31; i > -1; i--)
    {
        printf("%d",binaryArr[i]);
    }
    printf("\n");

    /*xor-division binaryArr/G(x) == 0 if ok*/
}

int getFrameErrorPercentage()
{
    char buffer[10];
    int percentage;

    printf("Enter the probability, in percentage (0-100), that a frame will be currupted/lost: ");
    fflush(stdin);
    fgets(buffer, 10, stdin);
    percentage = atoi(buffer);

    if(percentage < 0)
        percentage = 0;
    if(percentage > 100)
        percentage = 100;

    printf("Errorpercentage %d \n", percentage);
    return percentage;
}

int generateError(int chanceOfFrameError)
{
    int error = (rand() % 101);

    if(error <= chanceOfFrameError)/*frame will get error*/
    {
        return (rand() % 2);
    }
    else/*frame will be ok*/
    {
        return 0;
    }
}

