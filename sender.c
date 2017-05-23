#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include "header.h"

MsgList *head = NULL;
int sendPermission = 0;
int connectionId = 0;
int windowSize = 0;
int seqStart = 0;
int connectionPhase = 0;
MsgList node;
pthread_mutex_t mutex;
clock_t timerStart = 0;
clock_t timerStop = 0;
clock_t roundTripTime = 0;

struct sockaddr_in remaddr;
char *server = "127.0.0.1";	/* change this to use a different server */
int fd;
socklen_t addrlen = sizeof(remaddr);

int createSock();
void initSockReceiveOn(int fd, int port);
void initSockSendTo(int port);
void * connectionThread(void * fdSend);
void * sendThread(void * arg);
void * receiveThread(void * arg);

int main(void)
{
	pthread_t reader, writer;
	//socklen_t slen = sizeof(remaddr);
	//int recvlen;		/* # bytes in acknowledgement message */
	DataHeader data;
	createDataHeader(0, 0, 0, 0, 0, "helloFromSender!", &data);

	/* create a socket */
	pthread_mutex_init(&mutex, NULL);
	if((fd = createSock()) == 0)
	{
		printf("fd not created\n");
		exit(EXIT_FAILURE);
	}
	initSockReceiveOn(fd, 0);
	initSockSendTo(PORT);

	// if (sendto(fd, &data, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, slen)==-1)
	// {
	// 	printf("Error in sendto|n");
	// 	fflush(stdout);
	// 	exit(EXIT_FAILURE);
	// }
	// recvlen = recvfrom(fd, &data, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, &slen);
  // if (recvlen >= 0)
	// {
	// 	printf("received message: \"%s\" (%d bytes)\n", data.data, recvlen);
	// 	fflush(stdout);
  // }
	// recvlen = recvfrom(fd, &data, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, &slen);
	// if (recvlen >= 0)
	// {
	// 	printf("received message: \"%s\" (%d bytes)\n", data.data, recvlen);
	// 	fflush(stdout);
	// }

  pthread_create(&reader, NULL, receiveThread, NULL);
  pthread_create(&writer, NULL, connectionThread, NULL);
  pthread_exit(NULL);

  sendPermission = 0;
  connectionId = 0;
  windowSize = 0;
  seqStart = 0;
  connectionPhase = 0;
  printf("Connection closed\n");
	close(fd);
	return 0;
}
//////////////////////////////////////////////////////////////////////////
int createSock()
{
	int fd;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}
	return fd;
}
void initSockReceiveOn(int fd, int port)
{
	struct sockaddr_in myaddr;	/* our address */
	/* bind it to all local addresses and pick any port number */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
	{
		printf("bind failed");
		exit(EXIT_FAILURE);
	}
}
void initSockSendTo(int port)
{
	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */

	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(port);
	if (inet_aton(server, &remaddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
}

//////////////////////////////////////////////////////////////////////////

void * connectionThread(void *arg)
{
  DataHeader syn;
  DataHeader synack;
  DataHeader fin;
  DataHeader finack;
  MsgList *currentNode = NULL;
	createDataHeader(0, 0, 0, 0, getCRC(strlen("SYN"), "SYN"), "SYN", &syn);
	createDataHeader(1, connectionId, seqStart, windowSize, getCRC(strlen("SYNACK"), "SYNACK"), "SYNACK", &synack);
	createDataHeader(3, connectionId, 0, windowSize, getCRC(strlen("FIN"), "FIN"), "FIN", &fin);
	createDataHeader(4, connectionId, 0, windowSize, getCRC(strlen("FINACK"), "FINACK"), "FINACK", &finack);

  ////////////////////////////////////SYN////////////////////////////////////////////////
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
    if (sendto(fd, &syn, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
    {
      printf("syn failed\n");
      exit(EXIT_FAILURE);
    }
    printf("Sent SYN\n");
    sleep(10);
  }
  ////////////////////////////////////SYNACKACK////////////////////////////////////////////
  while(connectionPhase == 1)
  {
    connectionPhase = 2;
    //Send synackack to server
		if (sendto(fd, &synack, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
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

	//head = (MsgList*)malloc(sizeof(MsgList));
  while (1)
  {
    if (pthread_mutex_trylock(&mutex))
    {
      head = createMessages(head, connectionId, seqStart+1, windowSize);
      pthread_mutex_unlock(&mutex);
      break;
    }
  }
	currentNode = head;
  while(head != NULL)
  {
    while (1)
    {
			//printf("while\n");
			fflush(stdout);
      if (pthread_mutex_trylock(&mutex))
      {
        if(sendPermission < windowSize && currentNode != NULL)
        {
					printf("msg sent\n");
					fflush(stdout);
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
  node.sent = 0;
  node.acked = 0;
  node.data = &fin;
  node.next = NULL;
  pthread_create(&node.thread, NULL, sendThread, (void*)&node);
  pthread_join(node.thread, NULL);

  connectionPhase = 4;
  while (connectionPhase == 4)
  {
    ;
  }

  while(connectionPhase == 5)
  {
    connectionPhase = 6;
    //Send finack to server
		if (sendto(fd, &finack, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
    {
      printf("finack failed\n");
      exit(EXIT_FAILURE);
    }
    printf("finack sent\n");
    //wait timer
    //clock_t timer = clock() + roundTripTime;
    //while (clock() < timer);
	  sleep(10);
  }
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
          if (sendto(fd, ((MsgList*)arg)->data, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
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
    //clock_t timer = clock() + roundTripTime;
    //while (clock() < timer);
	sleep(10);
  }
  return NULL;
}

void * receiveThread(void * arg)
{
  int bytesReceived = 0;
  DataHeader buffer;
  printf("In receiveThread\n");
	fflush(stdout);

  while(connectionPhase < 6)
  {
    printf("before recvfrom\n");
		fflush(stdout);
    bytesReceived = recvfrom(fd, &buffer, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, &addrlen);
    //Add check for address that we received from
	  printf("flag: %d. msg from recv: %s\n", buffer.flag, buffer.data);
	  printf("connectionPhase: %d\n", connectionPhase);
		fflush(stdout);
    if (bytesReceived > 0 && (calcError(buffer.crc, strlen(buffer.data), buffer.data)) == 0)
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
            printf("Sender connected with id: %d and messages created with window size: %d\n", connectionId, windowSize);
          }
          //receiver timer must have been triggered and our SYNACK must have been lost
          else if(connectionPhase == 2)
          {
            connectionPhase = 1;
          }
          break;
        case 2:
          //MSGACK
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
  return NULL;
}
