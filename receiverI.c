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

#define PORT 5555
#define HOST_NAME_LENGTH 50
#define BUFSIZE 2048



//int threadCreate (void * functionCall, int threadParam, void * args);
int createSock();
void initSock(struct sockaddr_in *myaddr, int fd, int port);

int main(int argc, char *argv[])
{
    int fd;
    struct sockaddr_in sock;
    //For responding
    struct sockaddr_in remaddr;     /* remote address */
    socklen_t addrlen = sizeof(remaddr);            /* length of addresses */
    int recvlen;                    /* # bytes received */
    unsigned char buf[BUFSIZE];     /* receive buffer */

    fd = createSock();
    initSock(&sock, fd, PORT);

    printf("Socket created and initiated!\n");

    while(1)
    {
        printf("waiting on port %d\n", PORT);
        recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
        printf("received %d bytes\n", recvlen);
        if (recvlen > 0)
        {
                buf[recvlen] = 0;
                printf("received message: \"%s\"\n", buf);
        }
    }
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

void initSock(struct sockaddr_in *myaddr, int fd, int port)
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

// int main ()
// {
// 	DataHeader * incommingMsg;
// 	struct sockaddr * senderAddr;
// 	socklen_t * senderSockLen;
//     //int synRecv;
//     //readFrom, listen
// 	while (1)
// 	{
// 		int synRecv = recvfrom(PORT, incommingMsg, sizeof(DataHeader), 0, senderAddr, senderSockLen);
//
// 		//error check
// 		if (!synRecv)
// 		{
// 		    printf("Error from listen for syn: %s\n", strerror(errno) );
// 		    exit(EXIT_FAILURE);
// 		}
// 		else
// 		{
// 		    //check data
// 			switch (incommingMsg->flags)
// 			{
// 				//received syn
// 				case 0:
//
// 					break;
//
// 				//recived ack on syn-ack
// 				case 1:
// 					//
// 					break;
// 				//received new msg
// 				case 2:
// 					//save msg and send ack
// 					break;
// 				// received fin
// 				case 3:
// 					//send finack
// 					break;
// 				//recived fin ack
// 				case 4:
// 					break;
// 			}
// 			//else recvfrom
// 		}
// 	}
//   return(EXIT_SUCCESS);
// }


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
