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
//#define HOST_NAME_LENGTH 50
//#define BUFSIZE 2048
#define WINDOWSIZE 3

//funcs
void * listenFunc(void * args);
void * handleMsg (void * args);
int createSock();
void initSock(int fd, int port);
void fillArrWithSeq0(AcceptedClients * client);
void addToArr(AcceptedClients * client, DataHeader * incommingMsg, int index);

//global variables
//char hostName[HOST_NAME_LENGTH];
DataHeader window[(WINDOWSIZE-1)];
//ListHead * head;
AccClientListHead * clients;
pthread_mutex_t mutex;

//void * startUpFunc ()
int main(int argc, char *argv[])
{
	pthread_mutex_init(&mutex, NULL);
	//variables
	//head = createListHead();
	pthread_t listener;
	ArgForThreads args;

	// if(argv[1] == NULL)
  //   {
  //     printf("Error: no host name\n");
  //     exit(EXIT_FAILURE);
  //   }
  //   else
  //   {
  //     strncpy(hostName, argv[1], HOST_NAME_LENGTH);
  //     hostName[HOST_NAME_LENGTH - 1] = '\0';
  //   }

	clients = (AccClientListHead*)malloc(sizeof(AccClientListHead));
	clients->head = NULL;

  srand(time(NULL));

	//Create socket
  args.addrlen = sizeof(args.remaddr);
	if((args.fd = createSock()) == 0)
	{
		printf("fd not created\n");
		exit(EXIT_FAILURE);
	}
	initSock(args.fd, PORT);
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
	free(clients);
	return 0;
}


void * listenFunc(void * args)
{
	ArgForThreads * bla = NULL;

	struct sockaddr_in senderAddr;
	socklen_t addrlen = sizeof(senderAddr);
int crc;
	pthread_t msg;
	int msgRecv;
	while (1)
	{
		bla = (ArgForThreads *) malloc(sizeof(ArgForThreads));
		
		if( bla == NULL)
		{
			printf("FFFFFFFFFFF\n");
			fflush(stdout);
		}

		*bla  = *((ArgForThreads*) args);

		printf("\n\n\n-----Listening for msgs----- \n\n");
		fflush(stdout);
		msgRecv = recvfrom(bla->fd, &(bla->incommingMsg), sizeof(DataHeader), 0, (struct sockaddr *) &senderAddr, (socklen_t*)&(addrlen));

		//error check
		if (!msgRecv)
		{
			printf("Error from listenFunc for msg: %s\n", strerror(errno) );
			fflush(stdout);
		}

		else
		{
			bla->remaddr = senderAddr;
			bla->addrlen = addrlen;
			printf("msg from client: %s\n", bla->incommingMsg.data);
			printf("seq: %d", bla->incommingMsg.seq);
			fflush(stdout);
			crc = (calcError(bla->incommingMsg.crc, strlen(bla->incommingMsg.data), bla->incommingMsg.data)); 
			if( crc == 0)
			{
				printf("flag = %d\n", bla->incommingMsg.flag);
				fflush(stdout);
				int err = pthread_create(&msg, NULL, handleMsg, bla);
				if(err != 0)
				{
					 printf ("cant create thread: %s\n", strerror(errno));
					fflush(stdout);
					exit(EXIT_FAILURE);
				 }
				 else
				 {
					 //printf ("Successfully created thread!!!\n");
				 }
			}
			else
			{
				printf("wrong crc = %d in seq = %d", crc, bla->incommingMsg.seq); 
			}
		}
	//	sleep(1);
		//usleep(1000);
		//free(bla);
	}

	//bla = NULL;
	return (void *) 1;
}


void * handleMsg (void * args)
{
	printf("handleMSG\n");
	fflush(stdout);
	int sent;
	DataHeader * outgoingMsg = (DataHeader *) malloc(sizeof(DataHeader));
	ArgForThreads * temp = (ArgForThreads*)malloc(sizeof(ArgForThreads));

	*temp = *((ArgForThreads*) args);

	DataHeader * incommingMsg = (DataHeader *) malloc (sizeof(DataHeader));
	incommingMsg->seq = temp->incommingMsg.seq;
	incommingMsg->flag =  temp->incommingMsg.flag;
	incommingMsg->id =  temp->incommingMsg.id;
	incommingMsg->windowSize =  temp->incommingMsg.windowSize;
	incommingMsg->crc =  temp->incommingMsg.crc;
	strcpy(incommingMsg->data,temp->incommingMsg.data);

					printf("flag = %d\n", incommingMsg->flag);
				fflush(stdout);
	
	struct sockaddr_in tempAddr = temp->remaddr;

	unsigned int r = 0;
	int i;
	char * msg;
	AcceptedClients * tempClient;

	free(args);
	
	switch (incommingMsg->flag)
	{
		//syn received
		case 0:
			printf("case 0\n");
			fflush(stdout);
			msg = "This is an SynAck";
			while(r == 0)
			{
				//lägg in functioen för att kolla om r e lika något annat id
				r = rand();
			}

			while(1)
			{
				if(pthread_mutex_trylock(&mutex))
				{
					if(clients == NULL)
					{
						tempClient = NULL;
					}
					else
					{
						tempClient = findClient(clients->head, temp->remaddr, incommingMsg->id );
					}

					pthread_mutex_unlock(&mutex);
					break;
				}
			}

			if (tempClient != NULL )
			{
				createDataHeader(1, tempClient->id, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg);
				sent = sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen);
				//error check
				if (!sent)
				{
					printf("Error from case 0.1 for msg: %s\n", strerror(errno) );
					fflush(stdout);
				}
			}
			else
			{

				createDataHeader(1, r, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg);

				sent = sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen);
				//error check
				if (!sent)
				{
					printf("Error from 0.2 for msg: %s\n", strerror(errno) );
					fflush(stdout);
				}

				while(1)
				{
					if(pthread_mutex_trylock(&mutex))
					{
						addClient(clients, temp->remaddr, r);
						pthread_mutex_unlock(&mutex);
						break;
					}
				}
			}

			while(1)
			{
				if(pthread_mutex_trylock(&mutex))
				{
					tempClient = findClient(clients->head, temp->remaddr, r );
					pthread_mutex_unlock(&mutex);
					break;
				}
			}

			while (tempClient->synAckAck != 1)
			{
				sleep(2);

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
						//printf("No synackack resend\n");
						//fflush(stdout);
						msg = "This is an SynAck";
						createDataHeader(1, temp->incommingMsg.id, temp->incommingMsg.seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg);
						sent = sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen);
						//error check
						if (!sent)
						{
							printf("Error from 0.3 for msg: %s\n", strerror(errno) );
							fflush(stdout);
						}
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
				tempClient->synAckAck = 1;

				while(1)
				{
					if(pthread_mutex_trylock(&mutex))
					{
						tempClient->timerTime = 2 * (clock() - tempClient->timerTime);

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
					break;
				}
			}

			if (tempClient != NULL )
			{
				
				if(tempClient->synAckAck == 1)
				{
					printf("in seq: %d\n", incommingMsg->seq);
					printf("expecting seq: %d\n", tempClient->nextInSeq);
					fflush(stdout);

					if(incommingMsg->seq <= tempClient->nextInSeq-WINDOWSIZE)
					{
						break;
					}
					//recv the correct msg
					if(incommingMsg->seq == tempClient->nextInSeq)
					{
						//printf("msg i want\n");
						//fflush(stdout);
						msg = "This is an Ack";
						createDataHeader(2,  incommingMsg->id, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg, outgoingMsg);
						sent = sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen));
						//error check
						if (!sent)
						{
							printf("Error from case 2.1 for msg: %s\n", strerror(errno) );
							fflush(stdout);
						}
						while(1)
						{
							if(pthread_mutex_trylock(&(tempClient->mutex)))
							{
						//		printf("msg add \n");
						//		fflush(stdout);
								addMsgToClient(tempClient, incommingMsg);
								tempClient->nextInSeq = incommingMsg->seq + 1;

								for(i = 0; i < (WINDOWSIZE-1); i++)
								{
									if (tempClient->window[i].seq != 0)
									{
										addMsgToClient(tempClient, &(tempClient->window[i]));
										tempClient->nextInSeq = tempClient->window[i].seq + 1;
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
						//printf("msg + 1\n");
					//	fflush(stdout);
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
						sent = sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen));
						//error check
						if (!sent)
						{
							printf("Error from 2.2 for msg: %s\n", strerror(errno) );
							fflush(stdout);
						}
					}

					//recv not the next in seq but +2
					else if (incommingMsg->seq == (tempClient->nextInSeq+2))
					{
						//printf("msg +2\n");
						//fflush(stdout);
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
						sent = sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen));
						//error check
						if (!sent)
						{
							printf("Error from 2.3 for msg: %s\n", strerror(errno) );
							fflush(stdout);
						}
					}

					//ack on msg got lost send again
					else if (incommingMsg->seq < tempClient->nextInSeq)
					{
						//printf("msg i got\n");
						//fflush(stdout);
						msg = "This is an Ack, i alredy have this one";
						createDataHeader(2,  incommingMsg->id, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg);
						sent = sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen));
						//error check
						if (!sent)
						{
							printf("Error from case 2.4 for msg: %s\n", strerror(errno) );
							fflush(stdout);
						}
					}

					else
					{
						printf("msg i do not want\n");
						fflush(stdout);
						//msg that i do not want or can store
					}
				}
				else
				{
					/*//resend synAck because have not gotten synAckAck, but client thinks that connection is complet
					msg = "This is an synAck";
					createDataHeader(1, r, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg, outgoingMsg);
					sent = sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen);
					//error check
					if (!sent)
					{
						printf("Error from case 2.5 for msg: %s\n", strerror(errno) );
						fflush(stdout);
					}*/
				}
			}
			else
			{
				//a client that have not established connection is trying to send a msg
			}
			break;

		//received fin
		case 3:
			printf("Case 3\n");
			fflush(stdout);
			while(1)
			{
				printf("Bläh\n");
				fflush(stdout);
				if(pthread_mutex_trylock(&mutex))
				{
					printf("get tempClient\n");
					fflush(stdout);
					tempClient = findClient(clients->head, temp->remaddr, incommingMsg->id );
					pthread_mutex_unlock(&mutex);
					break;
				}
			}

			if (tempClient != NULL )
			{
				//printf("TempClient ! = NULL \n");
				//fflush(stdout);
				if(tempClient->synAckAck == 1)
				{
					msg = "This is an FinAck";
					createDataHeader(4,  incommingMsg->id, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg);
					sent = sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen));
				//	printf("header sent\n");
					//fflush(stdout);
					//error check
					if (!sent)
					{
						printf("Error from 3 for msg: %s\n", strerror(errno) );
						fflush(stdout);
					}


					clock_t time = (clock_t )tempClient->timerTime;
					clock_t currentTime = clock() + time;
					//printf("after set currentTime\n");
					//fflush(stdout);
					while(1)
					{
						while (clock() < currentTime);

						if(tempClient->finAck == 1)
						{
							break;
						}

						msg = "This is an Fin ";
						createDataHeader(3,  temp->incommingMsg.id, temp->incommingMsg.seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg, outgoingMsg);
						sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen);
						//printf("resent fin\n");
						fflush(stdout);
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
			if(tempClient == NULL)
			{
				break;
			}
			tempClient->finAck = 1;
			printMsg(tempClient->msgs);
			removeClient(clients,tempAddr,incommingMsg->id);
			break;
	}
	free(temp);
	free(incommingMsg);
	free(outgoingMsg);

	return (void *) 1;
}
///////////////////////////////Connection stuff/////////////////////////////////////////////////////////
int createSock()
{
	int fd;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}
	return fd;
}
void initSock(int fd, int port)
{
	struct sockaddr_in myaddr;	/* our address */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(PORT);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
	{
		printf("bind failed");
		exit(EXIT_FAILURE);
	}
}
//SocketFuncs
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
