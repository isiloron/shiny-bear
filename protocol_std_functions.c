/*
Data communication Spring 2014
Lab 3 - Reliable Transportation Protocol
Students: sdn08003
          lja08001
*/

#include "protocol_std.h"


void prepareSocket(int* sock_fd, struct sockaddr_in* sockaddr)
{
    /*prepare and bind socket to a port*/
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
    /*create new empty frame*/
    rtp* frame = malloc(sizeof(rtp));
    if(frame==NULL)
    {
        perror("Failed to create frame \n");
        exit(EXIT_FAILURE);
    }

    frame->flags = flags;
    frame->seq = seq;
    frame->data = data;

    return frame;
}


struct Buffer* newBuffer()
{
    /*create new buffer for serializing*/
    struct Buffer* buf = malloc(sizeof(struct Buffer));
    if(buf == NULL)
    {
        perror("Failed to create buffer!");
        exit(EXIT_FAILURE);
    }

    buf->data = malloc(BUFFERSIZE);
    buf->data = memset(buf->data,0,sizeof(BUFFERSIZE));
    buf->size = BUFFERSIZE;
    buf->next = 0;

    return buf;
}

void serializeFrame(rtp* frame, struct Buffer* buf)
{
    serialize_uint8(frame->flags,buf);
    serialize_uint8(frame->seq,buf);
    serialize_char(frame->data,buf);
}

void serialize_uint8(uint8_t n, struct Buffer* buf)
{
    /*the integer n is copied to the subsequent empty memory on the buffer*/
    memcpy( ((char *)buf->data)+(buf->next), &n, sizeof(uint8_t));
    //next empty memory is updated
    buf->next += sizeof(uint8_t);
}

void serialize_char(char c, struct Buffer* buf)
{
    /*the char c is copied to the subsequent empty memory on the buffer*/
    memcpy(((char*)buf->data)+(buf->next),&c, sizeof(char));
    //next empty memory is updated
    buf->next += sizeof(char);
}

void deserializeFrame(rtp* frame, struct Buffer* buf)
{
    deserialize_uint8(&(frame->flags),buf);
    deserialize_uint8(&(frame->seq),buf);
    deserialize_char(&(frame->data),buf);
}

void deserialize_uint8(uint8_t* n, struct Buffer* buf)
{
    //memory from buffer is copied to an integer
    //this is basically the reverse of the serializing function
    memcpy(n,((char*)buf->data)+(buf->next), sizeof(uint8_t));
    buf->next += sizeof(uint8_t);
}

void deserialize_char(char* c, struct Buffer* buf)
{
    //memory from buffer is copied to a char
    //this is basically the reverse of the serializing function
    memcpy(c,((char*)buf->data)+(buf->next), sizeof(char));
    buf->next += sizeof(char);
}

void sendFrame(int socket, rtp* frame, struct sockaddr_in dest, int chanceOfFrameError)
{
    /*This function takes a frame struct, transfers it to a buffer and sends
     *it over UDP, returns the number of bytes sent*/
    int error = generateError(chanceOfFrameError);
    int scramble;
    int byte;

    if(error == 1)/*frame dissapear*/
    {
        printf("\nFrame dissapeared!\n\n");
    }
    else
    {
        struct Buffer* buffer = newBuffer();
        int bytesSent = 0;
        serializeFrame(frame, buffer);
        setCrc(buffer->data);
        if(error == 2)//bit corruption
        {
            printf("\nBit error occured!\n\n");
            uint8_t *tempBuf = (uint8_t*)buffer->data;
            scramble = rand();
            for(byte=0;byte<BUFFERSIZE;byte++)
            {
                tempBuf[byte] = tempBuf[byte]^( scramble >> (8*(3-byte) ) ); //each byte in the buffer is XORed with each byte in "scramble"
            }
        }
        bytesSent = sendto(socket, buffer->data, buffer->size, 0, (struct sockaddr*)&dest, sizeof(dest));
        if(bytesSent == -1)
        {
            perror("Failed to send UDP packet!");
            exit(EXIT_FAILURE);
        }
        free(buffer);
    }
}

rtp* receiveFrame(int socket, struct sockaddr_in* sourceAddr)
{
    /*This function receives a UDP packet and translates it to a frame*/
    rtp* frame = NULL;/*framepointer*/
    struct Buffer* buffer;/*help struct for serializing and deserializing*/
    socklen_t addrLen = sizeof(*sourceAddr);
    buffer = newBuffer();/*create helpbuffer for deserializing*/
    int bytesReceived;
    bytesReceived = recvfrom(socket, buffer->data, buffer->size, 0, (struct sockaddr*)sourceAddr, &addrLen);/*received serialized segment*/
    if(bytesReceived == -1)
    {
        perror("Failed to receive UDP packet!");
        exit(EXIT_FAILURE);
    }
    frame = newFrame(0, 0, 0);/*create empty frame*/

    if(checkCrc(buffer->data))//if the CRC checks through
    {
        deserializeFrame(frame, buffer);/*Deserialize the received segment stored in buffer into the created frame*/
    }
    else
    {
        printf("\nBit error detected!\n\n");
    }

    free(buffer);

    return frame;
}

void resetShortTimeout(struct timeval* shortTimeout)
{
    //this function resets the short timeout
    shortTimeout->tv_sec = 0;
    shortTimeout->tv_usec = 200000; /*200 millisec*/
}

int waitForFrame(int fd, struct timeval* shortTimeout)
{
    /*this function waits for a new packet to arrive on the socket*/
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

int generateError(int chanceOfFrameError)/*errorgenerator*/
{
    /*this function randomly chooses if an error occurs.
    it returns 0 for no error, 1 for a packet loss, and 2 for bit corruption*/
    int error = (rand() % 100)+1; // error percentage from 1% to 100 %

    if(error <= chanceOfFrameError)/*frame will get error*/
    {
        return (rand() % 2)+1; // randomizes value between 1 and 2
    }
    else/*frame will be ok*/
    {
        return 0;
    }
}

//CRC (code reference: http://www.barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code)
void initCrc()
{
    /*this function generates a table of all possible remainders when the numerator
    is a byte and the denominator is CRCPOLY. this table is used to quickly calculate the CRC value.*/
    uint8_t remainder;
    int numerator;
    uint8_t bit;
    for (numerator=0; numerator<256; numerator++) //loops through all possible bytes
    {
        remainder = numerator;
        for(bit=8; bit>0 ; bit--) //for each bit in the byte
        {
            if(remainder & TOPBIT) //if the first bit is a one
            {
                remainder = (remainder << 1) ^ CRCPOLY; //bitshift to the left then XOR with CRCPOLY
            }
            else //if the first bit is a zero
            {
                remainder = (remainder << 1); //bitshift the zero
            }
        }
        crcTable[numerator] = remainder; //put the calculated remainder in the table
    }
}

uint8_t getCrc(uint8_t *buffer)
{
    /*this function calculates the crc value of the buffer*/
    uint8_t data;
    uint8_t remainder=0;
    int byte;
    for(byte=0; byte<(BUFFERSIZE-1); byte++) //bytewize calculation using the generated table
    {
        data = buffer[byte] ^ remainder;
        remainder = crcTable[data];
    }
    return remainder;
}

void setCrc(uint8_t *buffer)
{
    /*This function calulates the crc value of the buffer and appends it to the buffer*/
    buffer[BUFFERSIZE-1] = getCrc(buffer);
}

bool checkCrc(uint8_t *buffer)
{
    /*this function calculates the crc value with the getCrc function and adds a step to calculate the remainder of the whole buffer*/
    uint8_t remainder = getCrc(buffer);
    if(crcTable[buffer[BUFFERSIZE-1]^remainder] == 0) //the remainder is zero, the buffer is error free
        return true;
    else
        return false;
}
