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
int seqStart = 0;                                     //TODO: set this if I want
int connectionPhase = 0;
MsgList node;
pthread_mutex_t mutex;

clock_t timerStart = 0;
clock_t timerStop = 0;
clock_t roundTripTime = 0;
clock_t timer = 0;

int createSock();
void initSockSendto(struct sockaddr_in *myaddr, int fd, int port, char *host);
void initSockReceiveOn(struct sockaddr_in *myaddr, int fd, int port);
void * connectionThread(void * fdSend);
void * sendThread(void * arg);
void * receiveThread(void * arg);
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

    pthread_mutex_init(&mutex, NULL);

    pthread_create(&reader, NULL, receiveThread, NULL);
    pthread_create(&writer, NULL, connectionThread, NULL);
    pthread_exit(NULL);

    sendPermission = 0;
    connectionId = 0;
    windowSize = 0;
    seqStart = 0;
    connectionPhase = 0;
    printf("Connection closed\n");

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
  createDataHeader(0, 0, 0, 0, getCRC(strlen("SYN"), "SYN"), "SYN", &syn);

  while(connectionPhase == 0)
  {
    while (1)
    {
      if (pthread_mutex_trylock(&mutex))
      {
        timerStart = clock();
        pthread_mutex_unlock(&mutex);
        break;
      }
    }
    //Send syn to server
    if (sendto(fdSend, &syn, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
    {
      printf("syn failed\n");
      exit(EXIT_FAILURE);
    }
    printf("Sent SYN\n");
    sleep(10);
  }

  ////////////////////////////////////SYNACKACK////////////////////////////////////////////
  //create SYNACK
  createDataHeader(1, connectionId, seqStart, windowSize, getCRC(sizeof("SYNACK"), "SYNACK"), "SYNACK", &synack);

  while(connectionPhase == 1)
  {
    connectionPhase = 2;
    //Send synackack to server
    if (sendto(fdSend, &syn, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
    {
      printf("syn failed\n");
      exit(EXIT_FAILURE);
    }
    printf("Sent SYNACK\n");
    //wait timer
    clock_t timer = clock() + roundTripTime;
    while (clock() < timer);
  }

  printf("\nConnected to receiver!\n");
  /////////////////////////////message sending/////////////////////////////////////////
  currentNode = head;
  while (1)
  {
    if (pthread_mutex_trylock(&mutex))
    {
      createMessages(head, connectionId, seqStart, windowSize);
      pthread_mutex_unlock(&mutex);
      break;
    }
  }
  while(head != NULL)
  {
    while (1)
    {
      if (pthread_mutex_trylock(&mutex))
      {
        if(sendPermission < windowSize && currentNode != NULL)
        {
          pthread_create(&currentNode->thread, NULL, sendThread, (void*)currentNode);
          currentNode = currentNode->next;
        }
        pthread_mutex_unlock(&mutex);
        break;
      }
    }
  }
  printf("All messages sent!\n");
  connectionPhase = 3;

  //////////////////////////closing connection//////////////////////////////////
  createDataHeader(3, connectionId, 0, windowSize, getCRC(sizeof("FIN"), "FIN"), "FIN", &fin);
  node.sent = 0;
  node.acked = 0;
  node.data = &fin;
  node.next = NULL;
  pthread_create(&node.thread, NULL, sendThread, (void*)&node);
  pthread_join(node.thread, NULL);

  while (connectionPhase == 4)
  {
    ;
  }
  createDataHeader(4, connectionId, 0, windowSize, getCRC(sizeof("FINACK"), "FINACK"), "FINACK", &fin);
  while(connectionPhase == 5)
  {
    connectionPhase = 6;
    //Send synackack to server
    if (sendto(fdSend, &finack, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
    {
      printf("finack failed\n");
      exit(EXIT_FAILURE);
    }
    printf("finack sent\n");
    //wait timer
    clock_t timer = clock() + roundTripTime;
    while (clock() < timer);
  }
  close(fdSend);

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
    while (1)
    {
      if (pthread_mutex_trylock(&mutex))
      {
        if((MsgList*)arg != NULL)
        {
          if (sendto(fdSend, ((MsgList*)arg)->data, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
          {
            printf("send failed\n");
            exit(EXIT_FAILURE);
          }
        }
        pthread_mutex_unlock(&mutex);
        break;
      }
    }
    //wait timer
    clock_t timer = clock() + roundTripTime;
    while (clock() < timer);
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
	fflush(stdout);

  fdReceive = createSock();
  initSockReceiveOn(&receiveOnSock, fdReceive, 5732);

	  printf("created recv socket\n");
	fflush(stdout);
  while(connectionPhase < 6)
  {
	    printf("before recvfrom\n");
	fflush(stdout);
    bytesReceived = recvfrom(fdReceive, &buffer, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, &addrlen);
    //Add check for address that we received from
	  printf("msg from recv: %s", buffer.data);
    if (bytesReceived > 0 && (calcError(buffer.crc, sizeof(buffer.data), buffer.data)) == 0)
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
            while (1)
            {
              if (pthread_mutex_trylock(&mutex))
              {
                timerStop = clock();
                roundTripTime = (timerStop - timerStart) * 2;
                connectionPhase = 1;
                windowSize = buffer.windowSize;
                connectionId = buffer.id;
                pthread_mutex_unlock(&mutex);
                break;
              }
            }
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
            while (1)
            {
              if (pthread_mutex_trylock(&mutex))
              {
                setAck(head, buffer.seq, windowSize);
                head = removeFirstUntilNotAcked(head, &sendPermission);
                pthread_mutex_unlock(&mutex);
                break;
              }
            }
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
  close(fdReceive);
  return NULL;
}
