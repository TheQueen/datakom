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
#include <time.h>
#include "header.h"

#define PORT 5555
#define HOST_NAME_LENGTH 50

char hostName[HOST_NAME_LENGTH];
MsgList *head = NULL;                                 //Needs lots of mutex -_-
struct sockaddr_in sendToSock;                        //The address/socket we send data to - needs to be here to be checked by receiveThread
int fdSend;
int sendPermission = 0;
int connectionId = 0;
int windowSize = 0;
int connected = 0;
int connectionNotClosed = 1;
int seqStart = 0;                                     //TODO: set this if I want
int connectionPhase = 0;
MsgList node;

clock_t timer = 0;                                    //TODO: make it a long int

int createSock();
void initSockSendto(struct sockaddr_in *myaddr, int fd, int port, char *host);
void initSockReceiveOn(struct sockaddr_in *myaddr, int fd, int port);
void * connectionThread(void * fdSend);
void * sendThread(void * arg);
void * receiveThread(void * fdReceive);
int errorCheck(DataHeader *buffer);

int main(int argc, char *argv[])
{
    pthread_t reader, writer;
    /* Check arguments */
    if(argv[1] == NULL)
    {
      printf("Error: no host name\n");
      exit(EXIT_FAILURE);
    }
    else
    {
      strncpy(hostName, argv[1], HOST_NAME_LENGTH);
      hostName[HOST_NAME_LENGTH - 1] = '\0';
    }

    pthread_create(&reader, NULL, receiveThread, NULL);
    pthread_create(&writer, NULL, connectionThread, NULL);
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

void * connectionThread(void *arg)
{
  DataHeader syn;
  DataHeader synack;
  DataHeader fin;
  DataHeader finack;
  MsgList *currentNode = NULL;

  fdSend = createSock();
  initSockSendto(&sendToSock, fdSend, PORT, hostName);

  ////////////////////////////////////SYN////////////////////////////////////////////////
  createDataHeader(0, 0, 0, 0, 0/*insert crc*/, "SYN", &syn);

  while(connectionPhase == 0)
  {
    //TODO: start clock
    //Send syn to server
    if (sendto(fdSend, &syn, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
    {
      printf("syn failed\n");
      exit(EXIT_FAILURE);
    }
    printf("Sent SYN\n");
    sleep(1);
  }

  //TODO: calculate timer make it a long
  //timer (long)= stop - start

  ////////////////////////////////////SYNACKACK////////////////////////////////////////////
  //create SYNACK
  createDataHeader(1, connectionId, 0, windowSize, 0/*insert crc*/, "SYNACK", &synack);

  while(connectionPhase == 1)
  {
    //TODO: start clock
    connectionPhase = 2;
    //Send synackack to server
    if (sendto(fdSend, &syn, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
    {
      printf("syn failed\n");
      exit(EXIT_FAILURE);
    }
    printf("Sent SYN\n");
    //TODO:sleep(timer);
  }

  printf("\nConnected to receiver!\n");
  /////////////////////////////message sending/////////////////////////////////////////
  currentNode = head;
  createMessages(head, connectionId, seqStart, windowSize);
  //TODO: mutexxxxx
  while(head != NULL)
  {
    if(sendPermission < windowSize && currentNode != NULL)
    {
      pthread_create(&currentNode->thread, NULL, sendThread, (void*)currentNode);
      currentNode = currentNode->next;
    }
  }

  printf("All messages sent!\n");
  connectionPhase = 3;

  //////////////////////////closing connection//////////////////////////////////
  createDataHeader(3, connectionId, 0, windowSize, 0/*insert crc*/, "FIN", &fin);
  node.sent = 0;
  node.acked = 0;
  node.data = &fin;
  node.next = NULL;
  pthread_create(&node.thread, NULL, sendThread, (void*)&node);
  pthread_join(node.thread, NULL);
  //TODO: start timer thread

  while (connectionPhase == 4)
  {
    ;
  }

  while(connectionPhase == 5)
  {
    //TODO: start clock
    connectionPhase = 6;
    //Send synackack to server
    if (sendto(fdSend, &finack, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
    {
      printf("finack failed\n");
      exit(EXIT_FAILURE);
    }
    printf("finack sent\n");
    //TODO:sleep(timer);
  }

  printf("Connection closed\n");

  return NULL;
}

void * sendThread(void * arg)
{
  int type = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if (type != 0)
	{
		printf("Error in type\n");
	}
  while(1)
  {
    //TODO: mutex
    if (sendto(fdSend, ((MsgList*)arg)->data, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
    {
      printf("send failed\n");
      exit(EXIT_FAILURE);
    }
    //TODO: sleep(timer);
  }
  return NULL;
}

void * receiveThread(void * arg)
{
  int fdReceive;
  int bytesReceived = 0;
  struct sockaddr_in remaddr;
  socklen_t addrlen = sizeof(remaddr);
  struct sockaddr_in receiveOnSock;                     //The address/socket we received on
  DataHeader buffer;
  printf("In receiveThread\n");

  fdReceive = createSock();
  initSockReceiveOn(&receiveOnSock, fdReceive, 0);

  while(connectionPhase < 6)
  {
    bytesReceived = recvfrom(fdReceive, &buffer, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, &addrlen);
    //Add check for address that we received from
    if (bytesReceived > 0 /*TODO: && errorCheck(&buffer)*/)
    {
      switch (buffer.flag)
      {
        case 0:
          //SYN - SHOULD NOT RECEIVE - DO NOTHING
          break;
        case 1:
          //SYNACK
          //if not connected then connect
          if(connectionPhase == 0)
          {
            //TODO: stop timer
            connectionPhase = 1;
            windowSize = buffer.windowSize;
            connectionId = buffer.id;
            printf("Sender connected with id: %d and messages creates with window size: %d\n", connectionId, windowSize);
          }
          //receiver timer must have been triggered and our SYNACK must have been lost
          else if(connectionPhase == 2)
          {
            connectionPhase = 1;
          }
          break;
        case 2:
          //MSGACK
          //check connectionId if 0 then dont do stuff if not 0 do stuff
          if(connectionPhase == 2 && head != NULL)
          {
            setAck(head, buffer.seq, windowSize);
            head = removeFirstUntilNotAcked(head, &sendPermission);
          }
          break;
        case 3:
          //FIN
          if(connectionPhase == 4)
          {
            connectionPhase = 5;
          }
          if (connectionPhase == 6)
          {
            connectionPhase = 5;
          }
          break;
        case 4:
          //FINACK
          if(connectionPhase == 3)
          {
            pthread_cancel(node.thread);
          }
          break;
        default:
          break;
      }
    }
  }
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
