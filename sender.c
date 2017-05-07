#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include "header.h"

#define PORT 5555
#define HOST_NAME_LENGTH 50

struct sockaddr_in sendToSock;                        //The address/socket we send data to
int sendPermission = 0;
int nextInSeq = 0;
int connectionId = 0;
int windowSize = 0;
int fdSend;
int fdReceive;



int createSock();
void initSockSendto(struct sockaddr_in *myaddr, int fd, int port, char *host);
void initSockReceiveOn(struct sockaddr_in *myaddr, int fd, int port);
void * sendThread(void * fdSend);
void * receiveThread(void * fdReceive);
int errorCheck(DataHeader *buffer);


int main(int argc, char *argv[])
{
    pthread_t reader, writer;
    struct sockaddr_in remaddr;                           //The address/socket we received data from (Needed for checking if really server)
    struct sockaddr_in receiveOnSock;                     //The address/socket we received on
    char hostName[HOST_NAME_LENGTH];
    socklen_t addrlen = sizeof(receiveOnSock);            /* length of addresses */
    int bytesReceived;                                    /* # bytes received */
    DataHeader *syn = NULL;
    DataHeader *buffer = NULL;                            //TODO: Remember if i need to malloc
    /* Check arguments */
    if(argv[1] == NULL)
    {
      perror("Usage: client [host name]\n");
      exit(EXIT_FAILURE);
    }
    else
    {
      strncpy(hostName, argv[1], HOST_NAME_LENGTH);
      hostName[HOST_NAME_LENGTH - 1] = '\0';
    }

    /////////////////////////create and init sockets/////////////////////////////

    fdSend = createSock();
    fdReceive = createSock();
    initSockSendto(&sendToSock, fdSend, PORT, hostName);
    initSockReceiveOn(&receiveOnSock, fdReceive, 0);
    printf("Socket created and initiated!\n");

    //////////////////////////////connection/////////////////////////////////

    createDataHeader(0, 0, 0, 0, 0, "", syn);

    while(1)
    {
      //Send syn to server
      if (sendto(fdSend, syn, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
      {
        printf("sendto failed\n");
        exit(EXIT_FAILURE);
      }
      printf("Sent SYN\n");
      printf("waiting for SYNACK\n");
      bytesReceived = recvfrom(fdReceive, buffer, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, &addrlen);
      //TODO: remember to time and
      if (bytesReceived > 0 && buffer->flag == 1 && errorCheck(buffer))
      {
        windowSize = buffer->windowSize;
        connectionId = buffer->id;
        break;
      }
      else
      {
        printf("No SYNACK: timer was triggerd or the message was not a SYNACK or there was some error in the crc-code.\n");
      }
    }

    //send synackack
    if (sendto(fdSend, buffer, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
    {
      printf("sendto failed\n");
      exit(EXIT_FAILURE);
    }
    printf("Sent ACK\n");
    //TODO: listen for a time - if nothing is received everything is fine - we can star sending
    //                        - if synack is received our synackack never arrived

    /////////////////////////////message sending///////////////////////////////////

    pthread_create(&writer, NULL, sendThread, NULL);
    pthread_create(&reader, NULL, receiveThread, NULL);
    pthread_exit(NULL);

    return (EXIT_SUCCESS);
}

int createSock()
{
    int fd = -1;

    if( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("cannot create socket\n");
        exit(EXIT_FAILURE);
    }
    return fd;
}

void initSockSendto(struct sockaddr_in *myaddr, int fd, int port, char *host)
{
    /* bind to an arbitrary return address */
    /* because this is the client side, we don't care about the address */
    /* since no application will initiate communication here - it will */
    /* just send responses */
    /* INADDR_ANY is the IP address and 0 is the socket */
    /* htonl converts a long integer (e.g. address) to a network representation */
    /* htons converts a short integer (e.g. port) to a network representation */

    struct hostent *hp;     /* host information */
    /* look up the address of the server given its name */
    hp = gethostbyname(host);
    if (!hp)
    {
    	perror("could not obtain address of\n");
    	exit(EXIT_FAILURE);
    }

    memset((char *)myaddr, 0, sizeof(*myaddr));
    myaddr->sin_family = AF_INET;
    myaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr->sin_port = htons(port);
    memcpy((void *)&myaddr->sin_addr, hp->h_addr_list[0], hp->h_length);

    // if (bind(fd, (struct sockaddr *)myaddr, sizeof(*myaddr)) < 0)
    // {
    //     perror("bind failed");
    //     exit(EXIT_FAILURE);
    //
    // }
}

void initSockReceiveOn(struct sockaddr_in *myaddr, int fd, int port)
{
    /* bind to an arbitrary return address */
    /* because this is the client side, we don't care about the address */
    /* since no application will initiate communication here - it will */
    /* just send responses */
    /* INADDR_ANY is the IP address and 0 is the socket */
    /* htonl converts a long integer (e.g. address) to a network representation */
    /* htons converts a short integer (e.g. port) to a network representation */


    memset((char *)myaddr, 0, sizeof(*myaddr));
    myaddr->sin_family = AF_INET;
    myaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr->sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)myaddr, sizeof(*myaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

void * sendThread(void *arg)
{
  ///////////////////////////////create msgs///////////////////////////////////
  DataHeader message[15];
  createDataHeader(2, connectionId, 0, windowSize, 0, "0", &message[0]);
  createDataHeader(2, connectionId, 1, windowSize, 0, "1", &message[1]);
  createDataHeader(2, connectionId, 2, windowSize, 0, "2", &message[2]);
  createDataHeader(2, connectionId, 3, windowSize, 0, "3", &message[3]);
  createDataHeader(2, connectionId, 4, windowSize, 0, "4", &message[4]);
  createDataHeader(2, connectionId, 5, windowSize, 0, "5", &message[5]);
  createDataHeader(2, connectionId, 6, windowSize, 0, "6", &message[6]);
  createDataHeader(2, connectionId, 7, windowSize, 0, "7", &message[7]);
  createDataHeader(2, connectionId, 8, windowSize, 0, "8", &message[8]);
  createDataHeader(2, connectionId, 9, windowSize, 0, "9", &message[9]);
  createDataHeader(2, connectionId, 10, windowSize, 0, "10", &message[10]);
  createDataHeader(2, connectionId, 11, windowSize, 0, "11", &message[11]);
  createDataHeader(2, connectionId, 12, windowSize, 0, "12", &message[12]);
  createDataHeader(2, connectionId, 13, windowSize, 0, "13", &message[13]);
  createDataHeader(2, connectionId, 14, windowSize, 0, "14", &message[14]);
  printf("In sendThread\n");

  // int i;
  // for(i = 0; i<8; i++)
  // {
  //   // if(freeWindowSlots > 0)
  //   // {
  //   //   /* send a message to the server */
  //   //   if (sendto(fd, msgHeader, sizeof(*msgHeader), 0, (struct sockaddr *)&sock, sizeof(sock)) < 0)
  //   //   {
  //   //     perror("sendto failed");
  //   //     return 0;
  //   //   }
  //   //   else
  //   //   {
  //   //     i++;
  //   //   }
  //   // }
  // }
  return NULL;
}

void * receiveThread(void * arg)
{
  int bytesReceived = 0;
  struct sockaddr_in remaddr;
  socklen_t addrlen = sizeof(remaddr);
  DataHeader *buffer = NULL;                            //TODO: Remember if i need to malloc
  printf("In receiveThread\n");
  while(1)
  {
    bytesReceived = recvfrom(fdReceive, buffer, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, &addrlen);
    //TODO: remember to time
    if (bytesReceived > 0 && buffer->flag == 2 && errorCheck(buffer) && buffer->id == connectionId)
    {

    }
    else
    {
      printf("No ACK: timer was triggerd or the message was not a ACK or there was some error in the crc-code.\n");
    }
    // switch ()
    // {
    //   case 0:
    //     break;
    //   case 1:
    //     break;
    //   case 2:
    //     break;
    //   case 3:
    //     break;
    // }
  }
  return NULL;
}

int errorCheck(DataHeader *buffer)
{
  return 1;
}

// int threadCreate (void * functionCall, int threadParam, void * args)
// {
//     //A function that uses the pthreads library to create a new thread
//     //It  takes the function in which we want to start executing the new thread, a kind of id and a argument to pass to the function
//     //On success it returns 0
//     int err =0;
//     err = pthread_create(&threadParam, NULL, functionCall, args);
//     if(err != 0)
//     {
//         printf ("cant create thread\n");
//     }
//     else
//     {
//         //printf ("Successfully created thread!!!\n");
//     }
//     return err;
// }
