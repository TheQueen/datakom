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
#define BUFSIZE 2048

//Global to be able to change value in different threads
int freeWindowSlots = 0;

//Skapa syn
//Skapa synackack
//Skapa array med meddelanden
//


//int threadCreate (void * functionCall, int threadParam, void * args);
int createSock();
void initSock(struct sockaddr_in *myaddr, int fd, int port, char *host);

int main(int argc, char *argv[])
{
    int fd;
    int windowSize = 0;
    struct sockaddr_in sock;
    char *my_message = "Hello server!\n";
    char hostName[HOST_NAME_LENGTH];
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

    fd = createSock();
    initSock(&sock, fd, PORT, hostName);

    printf("Socket created and initiated!\n");

    /////////////////////a work in progress///////////////////////////////
    DataHeader * syn;
    void createDataHeader(0, 0, 0, 0, 0, "", syn);
    //set windowsize to what you get in synAck
    windowSize = 3;
    freeWindowSlots = 3;
    //make user interactive input later
    DataHeader * message[8];
    void createDataHeader(2, 0, 1, windowSize, 0, "Hej ", message[0]);
    void createDataHeader(2, 0, 2, windowSize, 0, "hur ", message[1]);
    void createDataHeader(2, 0, 3, windowSize, 0, "mår ", message[2]);
    void createDataHeader(2, 0, 5, windowSize, 0, "du? ", message[3]);
    void createDataHeader(2, 0, 1, windowSize, 0, "Hej ", message[4]);
    void createDataHeader(2, 0, 2, windowSize, 0, "hur ", message[5]);
    void createDataHeader(2, 0, 3, windowSize, 0, "mår ", message[6]);
    void createDataHeader(2, 0, 5, windowSize, 0, "du? ", message[7]);

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

void initSock(struct sockaddr_in *myaddr, int fd, int port, char *host)
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

void * sendThread(void * arg)
{
  for(int i = 0; i<8; i++)
  {
    if(freeWindowSlots > 0)
    {
      /* send a message to the server */
      if (sendto(fd, msgHeader, sizeof(*msgHeader), 0, (struct sockaddr *)&sock, sizeof(sock)) < 0)
      {
        perror("sendto failed");
        return 0;
      }
    }
  }
}

void * receiveThread(void * arg)
{
  while(1)
  {
    switch (flag)
    {
      case 0:
        break;
      case 1:
        break;
      case 2:
        break;
      case 3:
        break;
      case 4:
        break;
    }
  }

 	/*if (windowsize != 3 && syn == 1)
		{
			/* send a message to the server */
			/*if (sendto(fd, my_message, strlen(my_message), 0, (struct sockaddr *)&sock, sizeof(sock)) < 0)
			{
				perror("sendto failed");
				return 0;
			}
		}*/
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
