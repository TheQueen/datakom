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
	
	clients = (AccClientListHead*)malloc(sizeof(AccClientListHead)); 
	
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
	ArgForThreads * bla = NULL;
	
	struct sockaddr_in tempAddr;
	
	pthread_t msg; 
	int msgRecv;
	while (1) 
	{
		printf("hello\n");
		fflush(stdout);
		bla = (ArgForThreads *) malloc(sizeof(ArgForThreads));
		if( bla == NULL)
		{
			printf("FFFFFFFFFFF\n");
			fflush(stdout);
		}
		printf("1");
		fflush(stdout);
		bla  = (ArgForThreads*) args; 
		printf("4\n");
		fflush(stdout);
		tempAddr = bla->remaddr;
		
		printf("-----Listening for msgs----- \n\n\n");
		fflush(stdout);
		msgRecv = recvfrom(bla->fd, &(bla->incommingMsg), sizeof(DataHeader), 0, (struct sockaddr *) &tempAddr, (socklen_t*)&(bla->addrlen));

		//error check
		if (!msgRecv)
		{
			printf("Error from listenFunc for msg: %s\n", strerror(errno) );
			fflush(stdout);
		}

		else 
		{
			printf("prut\n");
			fflush(stdout);
			bla->sock = tempAddr; 
			printf("msg from client: %s\n", bla->incommingMsg.data);
			fflush(stdout);
			if((calcError(bla->incommingMsg.crc, strlen(bla->incommingMsg.data), bla->incommingMsg.data)) == 0)
			{
				printf("flag = %d\n", bla->incommingMsg.flag);
				fflush(stdout);
				pthread_create(&msg, NULL, handleMsg, bla); 
			}
		}
		
		
	}
	free(bla);
	bla = NULL;
	return (void *) 1; 
}


void * handleMsg (void * args)
{
	printf("handleMSG\n");
	fflush(stdout);
	DataHeader * outgoingMsg = (DataHeader *) malloc(sizeof(DataHeader));
	FinArg * synArgs = (FinArg*) malloc(sizeof(FinArg));
	FinArg * finArgs = (FinArg*) malloc(sizeof(FinArg));
	ArgForThreads * temp = (ArgForThreads*)malloc(sizeof(ArgForThreads));
	temp = (ArgForThreads*) args; 
	printf("flag = %d\n", temp->incommingMsg.flag);
	fflush(stdout);
	DataHeader * incommingMsg = (DataHeader *) malloc (sizeof(DataHeader));
	incommingMsg->flag =  temp->incommingMsg.flag;
	incommingMsg->id =  temp->incommingMsg.id;
	incommingMsg->windowSize =  temp->incommingMsg.windowSize;
	incommingMsg->crc =  temp->incommingMsg.crc;
	strcpy(incommingMsg->data,temp->incommingMsg.data);
	
	struct sockaddr_in tempAddr = temp->remaddr;
	unsigned int r = 0; 
	int i; 
	char * msg; 
	AcceptedClients * tempClient;
	printf("innan switch\n");
	fflush(stdout);
	printf("flag = %d\n", temp->incommingMsg.flag);
	fflush(stdout);
	printf("flag in local = %d\n", incommingMsg->flag);
	fflush(stdout);
	switch (0)//incommingMsg->flag)
	{
		//syn received
		case 0: 
			printf("case 0\n");
			fflush(stdout);
			msg = "This is an SynAck"; 
			while(r == 0)
			{
				printf("rand\n");
				fflush(stdout);
				r = rand(); 
			}
			
			while(1)
			{
				printf("while\n");
				fflush(stdout);
				if(pthread_mutex_trylock(&mutex))
				{
					printf("find client\n");
					fflush(stdout);
					if(clients == NULL)
					{
						tempClient = NULL;
					}
					else
					{
						tempClient = findClient(clients->head, temp->remaddr, incommingMsg->id );
					}
					printf("client found\n");
					fflush(stdout);
					pthread_mutex_unlock(&mutex);
					break;
				}
			}
			
			if (tempClient != NULL )
			{
				printf("tempClient != NULL\n");
				fflush(stdout);
				createDataHeader(1, tempClient->id, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg); 
				sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen); 
			}
			else
			{
				printf("client == NULL\n");
				fflush(stdout);
				createDataHeader(1, r, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg); 
				printf("dataheder\n");
				fflush(stdout);
				sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen); 
				printf("sent\n");
				fflush(stdout);
				
				while(1)
				{
					if(pthread_mutex_trylock(&mutex))
					{
						printf("add client\n");
						fflush(stdout);
						addClient(clients, temp->remaddr, r);
						printf("client added\n");
						fflush(stdout);
						pthread_mutex_unlock(&mutex);
						break;
					}
				}
			}
			
			while(1)
			{
				if(pthread_mutex_trylock(&mutex))
				{
					printf("find client\n");
					fflush(stdout);
					printf("blääääää\n");
					fflush(stdout);
					tempClient = findClient(clients->head, temp->remaddr, r );
					printf("clients found\n");
					fflush(stdout);
					pthread_mutex_unlock(&mutex);
					break; 
				}
			}
			printf("hello client: %d\n", tempClient->id );
			fflush(stdout);
			printf("win, synAckAck: %d\n", tempClient->synAckAck);
			fflush(stdout);
			printf("synargs\n");
			fflush(stdout);
			synArgs->args = temp;
			printf("temp\n");
			fflush(stdout);
			synArgs->client = tempClient;
			printf("client\n");
			fflush(stdout);
			synArgs->win = (int )WINDOWSIZE;
			printf("win, synAckAck: %d\n", tempClient->synAckAck);
			fflush(stdout);
			
			while (tempClient->synAckAck != 1)
			{
				printf("in synTimer\n");
				fflush(stdout);
				//while(clock() < currentTime && tempClient->synAckAck != 1);
				sleep(2);
				
				printf("timer done\n");
				fflush(stdout);
				
				if(tempClient->synAckAck == 1)
				{
					printf("synackack gotten\n");
					fflush(stdout);
					break; 
				}
				else
				{
					if(tempClient->synAckAck != 1)
					{
						printf("No synackack resend\n");
						fflush(stdout);
						msg = "This is an SynAck"; 
						createDataHeader(1, temp->incommingMsg.id, temp->incommingMsg.seq, finArgs->win, temp->incommingMsg.crc, msg , outgoingMsg); 
						sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen); 
					}
				}
			}

			break;
		//ack on the synAck recived
		case 1:
			while(1)
			{
				if(pthread_mutex_trylock(&mutex))
				{
					tempClient = findClient(clients->head, temp->remaddr, incommingMsg->id );
					pthread_mutex_unlock(&mutex);
					break;
				}
			}
			
			if(tempClient != NULL)
			{
				pthread_cancel(tempClient->syn); 
				
				while(1)
				{
					if(pthread_mutex_trylock(&mutex))
					{
						tempClient->timerTime = 2 * (clock() - tempClient->timerTime);
						tempClient->synAckAck = 1;
						fillArrWithSeq0(tempClient);
						tempClient->nextInSeq = incommingMsg->seq + 1;
						pthread_mutex_unlock(&mutex);
						break; 
					}
				}
			}
			
			break;
			
		//getMsg from client
		case 2:
			
			while(1)
			{
				if(pthread_mutex_trylock(&mutex))
				{
					tempClient = findClient(clients->head, temp->remaddr, incommingMsg->id );
					pthread_mutex_unlock(&mutex);
				}
			}
			
			if (tempClient != NULL )
			{
				if(tempClient->synAckAck == 1)
				{
					//recv the correct msg
					if(incommingMsg->seq == tempClient->nextInSeq)
					{
						msg = "This is an Ack";
						createDataHeader(2,  incommingMsg->id, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg, outgoingMsg); 
						sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 
						
						while(1)
						{
							if(pthread_mutex_trylock(&(tempClient->mutex)))
							{
								addMsgToClient(tempClient, incommingMsg); 


								for(i = 0; i < (WINDOWSIZE-1); i++)
								{
									if (tempClient->window[i].seq != 0)
									{
										addMsgToClient(tempClient, &(tempClient->window[i]));  
										tempClient->window[i].seq = 0; 
									}
								}
								pthread_mutex_unlock(&(tempClient->mutex));
								break; 
							}
						}
						
					}
					
					//recv not the next in seq but +1
					else if(incommingMsg->seq == (tempClient->nextInSeq+1))
					{
						msg = "This is an Ack, I'm missing one msg";
						
						while(1)
						{
							if(pthread_mutex_trylock(&(tempClient->mutex)))
							{
								addToArr(tempClient, incommingMsg, 0);
								pthread_mutex_unlock(&(tempClient->mutex));
								break; 
							}
						}
						
						createDataHeader(2,  incommingMsg->id, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg); 
						sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 
					}
					
					//recv not the next in seq but +2
					else if (incommingMsg->seq == (tempClient->nextInSeq+2))
					{
						msg = "This is an Ack, not accept any more";
						
						while(1)
						{
							if(pthread_mutex_trylock(&(tempClient->mutex)))
							{
								addToArr(tempClient, incommingMsg, 1);
								pthread_mutex_unlock(&(tempClient->mutex));
								break; 
							}
						}
						
						createDataHeader(2,  incommingMsg->id, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg); 
						sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 
					}
					
					//ack on msg got lost send again
					else if (incommingMsg->seq < tempClient->nextInSeq)
					{
						msg = "This is an Ack, i alredy have this one"; 
						createDataHeader(2,  incommingMsg->id, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg); 
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
					createDataHeader(1, r, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg, outgoingMsg); 
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
			
			while(1)
			{
				if(pthread_mutex_lock(&mutex))
				{
					tempClient = findClient(clients->head, temp->remaddr, incommingMsg->id );
					pthread_mutex_unlock(&mutex);
					break; 
				}
			}
			
			if (tempClient != NULL )
			{
				if(tempClient->synAckAck == 1)
				{
					msg = "This is an FinAck";
					createDataHeader(3,  incommingMsg->id, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg); 
					sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 

					 
					finArgs->args = temp;
					finArgs->args->incommingMsg = *incommingMsg;
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
			
			while(1)
			{
				if(pthread_mutex_trylock(&mutex))
				{
					tempClient = findClient(clients->head, temp->remaddr, incommingMsg->id );
					pthread_mutex_unlock(&mutex);
					break; 
				}
			}
			
			pthread_cancel(tempClient->fin); 
			printMsg(tempClient->msgs); 
			removeClient(clients,tempAddr,incommingMsg->id);
			break;
	}
	free(temp); 
	free(incommingMsg); 
	free(outgoingMsg);
	free(synArgs);
	free(finArgs);
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



void cpyArg(ArgForThreads * to, ArgForThreads * from)
{
	
}