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

clock_t start = 0;
clock_t stop = 0;
clock_t timer = 0;

int createSock();
void initSockSendto(struct sockaddr_in *myaddr, int fd, int port, char *host);
void initSockReceiveOn(struct sockaddr_in *myaddr, int fd, int port);
void * connectionThread(void * fdSend);
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
  //DataHeader fin;
  //DataHeader finack;
  //MsgList *currentNode = NULL;

  fdSend = createSock();
  initSockSendto(&sendToSock, fdSend, PORT, hostName);

  ////////////////////////////////////SYN////////////////////////////////////////////////
  createDataHeader(0, 0, 0, 0, 0/*insert crc*/, "SYN", &syn);

  while(connectionId == 0)
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

  //TODO: calculate timer
  //timer = stop - start

  ////////////////////////////////////SYNACK////////////////////////////////////////////
  //create SYNACK
  createDataHeader(1, connectionId, 0, windowSize, 0/*insert crc*/, "SYNACK", &synack);

  while(!connected)
  {
    //send synack
    if (sendto(fdSend, &synack, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
    {
      printf("synack failed\n");
      exit(EXIT_FAILURE);
    }
    connected = 1;
    printf("Sent SYNACK\n");
    fflush(stdout);
    //TODO: sleep(timer);
  }

  printf("\nConnected to receiver!\n");

  createMessages(head, connectionId, seqStart, windowSize);


  /////////////////////////////message sending//////////////////////////////////
  // while(head != NULL)
  // {
  //   // if(sendPermission < windowSize)
  //   // {
  //   //   //start sendThreads?
  //   // }
  // }
  //closepthreads

  //TODO: make sendThread with this
  // send message
  // while(1)
  // {
  //   send again
  //   usleep(timer/ehhhh);
  // }

  //windowSize = 0;

  //////////////////////////closing connection//////////////////////////////////
  // createDataHeader(3, connectionId, 0, windowSize, 0/*insert crc*/, "FIN", &fin);
  // while
  //
  //
  // createDataHeader(4, connectionId, 0, windowSize, 0/*insert crc*/, "FINACK", &finack);
  //
  // connectionId = 0;
  // connected = 0;
  // int connectionNotClosed = 0;

  return NULL;
}

void * sendThread(void * arg)
{
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

  while(connectionNotClosed)
  {
    bytesReceived = recvfrom(fdReceive, &buffer, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, &addrlen);
    //Add check for address that we received from
    if (bytesReceived > 0 && errorCheck(&buffer))
    {
      switch (buffer.flag)
      {
        case 0:
          //SYN - SHOULD NOT RECEIVE - DO NOTHING
          break;
        case 1:
          //SYNACK
          //if not connected then connect
          if(connectionId == 0)
          {
            //TODO: stop timer
            windowSize = buffer.windowSize;
            connectionId = buffer.id;
            connected = 1;
            printf("Sender connected with id: %d and messages creates with window size: %d\n", connectionId, windowSize);
          }
          //receiver timer must have been triggered and our SYNACK must have been lost
          else
          {
            connected = 0;
          }
          break;
        case 2:
          //MSGACK

          //check connectionId if 0 then dont do stuff if not 0 do stuff
          if(connected != 0 && head != NULL)
          {
            setAck(head, buffer.seq, windowSize);
            head = removeFirstUntilNotAcked(head, &sendPermission);
          }
          break;
        case 3:
          //FIN

          //check connectionId if 0 then dont do stuff if not 0 do stuff
          // if(connected != 0)
          // {
          //
          // }
          break;
        case 4:
          //FINACK

          //check connectionId if 0 then dont do stuff if not 0 do stuff
          // if(connected != 0)
          // {
          //
          // }
          break;
        default:
          break;
      }
    }
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
