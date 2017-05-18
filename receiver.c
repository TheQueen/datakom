//Receiver

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include "header.h"


#define PORT 5555
#define HOST_NAME_LENGTH 50
#define BUFSIZE 2048
#define WINDOWSIZE 3 





//funcs
void * listenFunc(void * args);
void * handleMsg (void * args);
int createSock();
void initSock(struct sockaddr_in *myaddr, int fd, int port); 
void fillArrWithSeq0(AcceptedClients * client);
void addToArr(AcceptedClients * client, DataHeader * incommingMsg, int index);



//global variables 

DataHeader window[(WINDOWSIZE-1)]; 
ListHead * head;
AccClientListHead * clients;
pthread_mutex_t mutex; 

//void * startUpFunc ()
int main(int argc, char *argv[])
{	
	pthread_mutex_init(&mutex, NULL);
	//variables
	head = createListHead();
	pthread_t listener; 
	ArgForThreads args; 
	
    srand(time(NULL));
	//Create socket
	
    args.addrlen = sizeof(args.remaddr);           
    args.fd = createSock();
    initSock(&(args.sock), args.fd, PORT);

    //printf("Socket created and initiated!\n");
	
	
	int err = pthread_create(&listener, NULL, listenFunc, &args); 
	if(err != 0)
    {
         printf ("cant create thread\n");
     }
     else
     {
         //printf ("Successfully created thread!!!\n");
     }
	
	pthread_exit(NULL);
	return 0; 		    
}


void * listenFunc(void * args)
{
	
	ArgForThreads * temp = (ArgForThreads*) args; 
	struct sockaddr_in tempAddr = temp->remaddr;
	pthread_t msg; 
	int msgRecv;
	while (1) 
	{
		printf("-----Listening for msgs----- \n");
		msgRecv = recvfrom(temp->fd, temp->incommingMsg, sizeof(DataHeader), 0, (struct sockaddr *) &tempAddr, (socklen_t*)&(temp->addrlen));

		//error check
		if (!msgRecv)
		{
			printf("Error from listenFunc for msg: %s\n", strerror(errno) ); 

		}

		else 
		{ 
			if((calcError(temp->incommingMsg->crc, strlen(temp->incommingMsg->data), temp->incommingMsg->data)) == 0)
			{
				pthread_create(&msg, NULL, handleMsg, args); 
			}
		}
		
	}
	return (void *) 1; 
}


void * handleMsg (void * args)
{
	
	DataHeader * outgoingMsg = NULL;
	FinArg * synArgs = NULL;
	FinArg * finArgs = NULL;
	
	ArgForThreads * temp = (ArgForThreads*) args; 
	struct sockaddr_in tempAddr = temp->remaddr;
	int r = 0; 
	int i; 
	char * msg; 
	AcceptedClients * tempClient;
	
	switch (temp->incommingMsg->flag)
	{
		//syn received
		case 0: 
			msg = "This is an SynAck"; 
			while(r == 0)
			{
				r = rand(); 
			}
			pthread_mutex_lock(&mutex);
			tempClient = findClient(clients->head, temp->remaddr, temp->incommingMsg->id );
			pthread_mutex_unlock(&mutex);
			
			if (tempClient != NULL )
			{
				createDataHeader(1, tempClient->id, temp->incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg); 
				sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen); 
			}
			else
			{
				createDataHeader(1, r, temp->incommingMsg->seq, WINDOWSIZE, temp->incommingMsg->crc, msg , outgoingMsg); 
				sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen); 
				
				pthread_mutex_lock(&mutex);
				addClient(clients, temp->remaddr, r);
				pthread_mutex_unlock(&mutex);
			}
			
			pthread_mutex_lock(&mutex);
			tempClient = findClient(clients->head, temp->remaddr, temp->incommingMsg->id );
			pthread_mutex_unlock(&mutex);
			
			 
			synArgs->args = temp; 
			synArgs->client = tempClient; 
			synArgs->win = (int )WINDOWSIZE; 
			
			int err = pthread_create(&(tempClient->syn), NULL, synTimer, synArgs); 
			if(err != 0)
			{
				 printf ("cant create thread\n");
			 }
			 else
			 {
				 //printf ("Successfully created thread!!!\n");
			 }

			pthread_exit(NULL);
			break;
		//ack on the synAck recived
		case 1:
			pthread_mutex_lock(&mutex);
			tempClient = findClient(clients->head, temp->remaddr, temp->incommingMsg->id );
			pthread_mutex_unlock(&mutex);
			
			if(tempClient != NULL)
			{
				pthread_cancel(tempClient->syn); 
				
				pthread_mutex_lock(&mutex);
				tempClient->timerTime = 2 * (clock() - tempClient->timerTime);
				tempClient->synAckAck = 1;
				fillArrWithSeq0(tempClient);
				tempClient->nextInSeq = temp->incommingMsg->seq + 1;
				pthread_mutex_unlock(&mutex);
			}
			
			break;
			
		//getMsg from client
		case 2:
			
			pthread_mutex_lock(&mutex);
			tempClient = findClient(clients->head, temp->remaddr, temp->incommingMsg->id );
			pthread_mutex_unlock(&mutex);
			
			if (tempClient != NULL )
			{
				if(tempClient->synAckAck == 1)
				{
					//recv the correct msg
					if(temp->incommingMsg->seq == tempClient->nextInSeq)
					{
						msg = "This is an Ack";
						createDataHeader(2,  temp->incommingMsg->id, temp->incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg, outgoingMsg); 
						sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 
						
						pthread_mutex_lock(&(tempClient->mutex));
						addMsgToClient(tempClient, temp->incommingMsg); 
						
						
						for(i = 0; i < (WINDOWSIZE-1); i++)
						{
							if (tempClient->window[i].seq != 0)
							{
								addMsgToClient(tempClient, &(tempClient->window[i]));  
								tempClient->window[i].seq = 0; 
							}
						}
						pthread_mutex_unlock(&(tempClient->mutex));
						
					}
					
					//recv not the next in seq but +1
					else if(temp->incommingMsg->seq == (tempClient->nextInSeq+1))
					{
						msg = "This is an Ack, I'm missing one msg";
						
						pthread_mutex_lock(&(tempClient->mutex));
						addToArr(tempClient, temp->incommingMsg, 0);
						pthread_mutex_unlock(&(tempClient->mutex));
						
						createDataHeader(2,  temp->incommingMsg->id, temp->incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg); 
						sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 
					}
					
					//recv not the next in seq but +2
					else if (temp->incommingMsg->seq == (tempClient->nextInSeq+2))
					{
						msg = "This is an Ack, not accept any more";
						
						pthread_mutex_lock(&(tempClient->mutex));
						addToArr(tempClient, temp->incommingMsg, 1);
						pthread_mutex_unlock(&(tempClient->mutex));
						
						createDataHeader(2,  temp->incommingMsg->id, temp->incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg); 
						sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 
					}
					
					//ack on msg got lost send again
					else if (temp->incommingMsg->seq < tempClient->nextInSeq)
					{
						msg = "This is an Ack, i alredy have this one"; 
						createDataHeader(2,  temp->incommingMsg->id, temp->incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg); 
						sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 
					}
					
					else
					{
						//msg that i do not want or can store
					}
				}
				else 
				{
					//resend synAck because have not gotten synAckAck, but client thinks that connection is complet
					msg = "This is an synAck";
					createDataHeader(1, r, temp->incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg, outgoingMsg); 
					sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen); 
				}
			}
			else
			{
				//a client that have not established connection is trying to send a msg
			}
			break;
			
		//received fin
		case 3:
			
			pthread_mutex_lock(&mutex);
			tempClient = findClient(clients->head, temp->remaddr, temp->incommingMsg->id );
			pthread_mutex_unlock(&mutex);
			
			if (tempClient != NULL )
			{
				if(tempClient->synAckAck == 1)
				{
					msg = "This is an FinAck";
					createDataHeader(3,  temp->incommingMsg->id, temp->incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg); 
					sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 

					 
					finArgs->args = temp; 
					finArgs->client = tempClient; 
					finArgs->win = (int )WINDOWSIZE; 
					
					int err = pthread_create(& (tempClient->fin), NULL, finTimer, finArgs); 
					if(err != 0)
					{
						 printf ("cant create thread\n");
					 }
					 else
					 {
						 //printf ("Successfully created thread!!!\n");
					 }

					pthread_exit(NULL);
					
				}
			}
		break;
		
		case 4:
			//received fin ack
			
			pthread_mutex_lock(&mutex);
			tempClient = findClient(clients->head, temp->remaddr, temp->incommingMsg->id );
			pthread_mutex_unlock(&mutex);
			
			pthread_cancel(tempClient->fin); 
			printMsg(tempClient->msgs); 
			removeClient(clients,tempAddr,temp->incommingMsg->id);
			break;
	}
	return (void *) 1; 
}



//Create socket funcs
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

//SocketFuncs

//to fill arr with "Empty" dataHeaders
void fillArrWithSeq0(AcceptedClients * client)
{
	int i = 0; 
	for (i = 0; i<(WINDOWSIZE-1);i++)
	{
		client->window[i].seq = 0;
	}
}

//
void addToArr(AcceptedClients * client, DataHeader * incommingMsg, int index)
{
	client->window[index].flag = incommingMsg->flag; 
	client->window[index].id = incommingMsg->id; 
	client->window[index].seq = incommingMsg->seq; 
	client->window[index].windowSize = incommingMsg->windowSize; 
	client->window[index].crc = incommingMsg->crc; 
	strcpy(client->window[index].data, incommingMsg->data);
}



