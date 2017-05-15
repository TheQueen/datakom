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
MsgList *head = NULL;
struct sockaddr_in sendToSock;                        //The address/socket we send data to
int sendPermission = 0;
//int nextInSeq = 0;
int connectionId = 0;
int windowSize = 0;
//
//clock_t start;
//clock_t stop;
//clock_t timer;

//Dont know windowSize - need to get it and constuct sliding window
//Make linked list with length of all messages
//send first three messages
//Id = id på kommunikationen

//IDAS_STRUCT
//ackad
//skickad
//tid

//connected

//JAg vill ha
//lägg till
//ta bort första
//sök tre


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
  int fdSend;
  DataHeader syn;

  fdSend = createSock();
  initSockSendto(&sendToSock, fdSend, PORT, hostName);


  createDataHeader(0, 0, 0, 0, 0/*insert crc*/, "Hello world!", &syn);

  while(connectionId == 0)
  {
    //TODO: start clock
    //Send syn to server
    if (sendto(fdSend, &syn, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
    {
      printf("sendto failed\n");
      exit(EXIT_FAILURE);
    }
    printf("Sent SYN\n");
    sleep(1);
  }



  //send synackack
  // if (sendto(fdSend, buffer, sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
  // {
  //   printf("sendto failed\n");
  //   exit(EXIT_FAILURE);
  // }
  printf("Sent ACK\n");
  fflush(stdout);
  //TODO: listen for a time - if nothing is received everything is fine - we can star sending
  //                        - if synack is received our synackack never arrived

  /////////////////////////////message sending///////////////////////////////////

  // printf("In sendThread\n");
  //
  //  int i;
  //  for(i = 0; i<messages; i++)
  //  {
  //    if(sendPermission == 0)
  //    {
  //      /* send a message to the server */
  //      if (sendto(fdSend, &message[i], sizeof(DataHeader), 0, (struct sockaddr *)&sendToSock, sizeof(sendToSock)) < 0)
  //      {
  //        perror("sendto failed");
  //        exit(EXIT_FAILURE);
  //      }
  //      else
  //      {
  //        i++;
  //      }
  //    }
  // }
  // printf("All messages sent!\n");

  //skicka om du fortfarande får
  //starta tråd per skickad och skicka igen om ingen ack/timeout
  //


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
  int connectionNotClosed = 1;

  fdReceive = createSock();
  initSockReceiveOn(&receiveOnSock, fdReceive, 0);

  while(connectionNotClosed)
  {
    bytesReceived = recvfrom(fdReceive, &buffer, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, &addrlen);
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
              //timer = stop = start;
              windowSize = buffer.windowSize;
              connectionId = buffer.id;
              createMessages(head, connectionId, windowSize);
              printf("Sender connected with id: %d and messages creates with window size: %d\n", connectionId, windowSize);
          }
          break;
        case 2:
          //MSGACK

          //check connectionId if 0 then dont do stuff if not 0 do stuff
          // if(connectionId != 0)
          // {
          //
          // }
          break;
        case 3:
          //FIN

          //check connectionId if 0 then dont do stuff if not 0 do stuff
          // if(connectionId != 0)
          // {
          //
          // }
          break;
        case 4:
          //FINACK

          //check connectionId if 0 then dont do stuff if not 0 do stuff
          // if(connectionId != 0)
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
