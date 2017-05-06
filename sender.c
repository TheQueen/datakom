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

int createSock();
void initSockSendto(struct sockaddr_in *myaddr, int fd, int port, char *host);
void initSockReceiveOn(struct sockaddr_in *myaddr, int fd, int port);

int main(int argc, char *argv[])
{
    int connectionId = 0;
    int fdSend;
    int fdReceive;
    int windowSize = 0;
    struct sockaddr_in sendToSock;
    struct sockaddr_in receiveOnSock;
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

    fdSend = createSock();
    fdReceive = createSock();
    initSockSendto(&sendToSock, fdSend, PORT, hostName);
    initSockReceiveOn(&receiveOnSock, fdReceive, 0);
    printf("Socket created and initiated!\n");

    /////////////////////a work in progress///////////////////////////////

    createDataHeader(0, 0, 0, 0, 0, "", syn);

    while(1)
    {
      //Send syn to server
      if (sendto(fdSend, syn, sizeof(*syn), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
      {
        printf("sendto failed\n");
        exit(EXIT_FAILURE);
      }
      printf("Sent SYN\n");
      printf("waiting for SYNACK\n");
      bytesReceived = recvfrom(fdReceive, buffer, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, &addrlen);

      if (bytesReceived > 0 && buffer->flag == 1/* && TODO: error check! */)
      {
        windowSize = buffer->windowSize;
        connectionId = buffer->id;
      }
    }

    //////////////////////////////////////////////////////////

    printf("Did some stuff!\n");


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

void * sendThread(void * arg)
{
  printf("Did some stuff!\n");

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
  printf("Did some stuff!\n");

  // while(1)
  // {
  //   switch (flag)
  //   {
  //     case 0:
  //       break;
  //     case 1:
  //       break;
  //     case 2:
  //       break;
  //     case 3:
  //       break;
  //     case 4:
  //       break;
  //   }
  // }
  //
	// if (windowsize != 3 && syn == 1)
	// {
	// if (sendto(fd, my_message, strlen(my_message), 0, (struct sockaddr *)&sock, sizeof(sock)) < 0)
	//   {
	// 	  perror("sendto failed");
	//     return 0;
  //   }
  // }
  return NULL;
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
