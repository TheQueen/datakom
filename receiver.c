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

int main(int argc, char *argv[])
{
	//variables
	head = createListHead();
	pthread_t listener;
	ArgForThreads args;

	//init stuff
	pthread_mutex_init(&mutex, NULL);
	clients = (AccClientListHead*)malloc(sizeof(AccClientListHead));
	//IDAS TILLÄG!
	clients->head = NULL;
  srand(time(NULL));
	//Create socket
  args.addrlen = sizeof(args.remaddr);		//why here? should be one for every client
  args.fd = createSock();
  initSock(&(args.sock), args.fd, PORT);

  printf("Socket created and initiated!\n");
	fflush(stdout);

	//create thread that listens, will run through whole program
	int err = pthread_create(&listener, NULL, listenFunc, &args);
	if(err != 0)
  {
		printf ("cant create thread\n");
		exit(EXIT_FAILURE);
	}
	else
	{
	   //printf ("Successfully created thread!!!\n");
	}
	pthread_exit(NULL);

	//TODO:borde tömma client och head och frigöra dom här
	return 0;
}

void * listenFunc(void * args)
{
	ArgForThreads * bla = NULL;
	//struct sockaddr_in senderAddr;
	//socklen_t addrlen = sizeof(senderAddr);

	//kan man använda bara en såhär för att skapa flera trådar?
	//Om man inte får - skapa lokal variabel i i början av whilen?
	pthread_t msg;
	int msgRecv;

	while (1)
	{
		printf("listenFunc top of while\n");
		fflush(stdout);
		bla = (ArgForThreads *) malloc(sizeof(ArgForThreads));
		if( bla == NULL)
		{
			printf("Could not malloc bla\n");
			fflush(stdout);
			// jag la till denna
			break;
		}
		printf("1\n");
		fflush(stdout);
		//borde kanske använda memcpy för att det finns en array i sockaddr_in
		bla  = (ArgForThreads*) args;
		printf("4\n");
		fflush(stdout);

		printf("-----Listening for msg----- \n\n\n");
		fflush(stdout);
		//tog bort två onödiga variabler och använde de i bla/args
		msgRecv = recvfrom(bla->fd, &(bla->incommingMsg), sizeof(DataHeader), 0, (struct sockaddr *) &bla->remaddr, (socklen_t*)&(bla->addrlen));
		if (!msgRecv)
		{
			printf("Error from listenFunc for msg: %s\n", strerror(errno) );
			fflush(stdout);
			//jag la till denna
			break;
		}
		else
		{
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
	//kommer aldrig komma hit om allt går bra så därför skicka -1 i stället?
	//men varför ens skicka något om du inte tar emot värdet med en join?
	return (void *) 1;
}

void * handleMsg (void * args)
{
	printf("handleMSG\n");
	fflush(stdout);

	int sent;
	DataHeader * outgoingMsg = (DataHeader *) malloc(sizeof(DataHeader));
	FinArg * synArgs = (FinArg*) malloc(sizeof(FinArg));
	FinArg * finArgs = (FinArg*) malloc(sizeof(FinArg));
	ArgForThreads * temp = (ArgForThreads*)malloc(sizeof(ArgForThreads));
	//borde kanske använda memcpy för att det finns en array i sockaddr_in
	temp = (ArgForThreads*) args;
	//printf("flag = %d\n", temp->incommingMsg.flag);
	//fflush(stdout);

	DataHeader * incommingMsg = (DataHeader *) malloc (sizeof(DataHeader));
	incommingMsg->flag =  temp->incommingMsg.flag;
	incommingMsg->id =  temp->incommingMsg.id;
	incommingMsg->windowSize =  temp->incommingMsg.windowSize;
	incommingMsg->crc =  temp->incommingMsg.crc;
	strcpy(incommingMsg->data,temp->incommingMsg.data);

	//borde kanske använda memcpy för att det finns en array i sockaddr_in
	struct sockaddr_in tempAddr = temp->remaddr;

	//int sendFd = createSock();
	//initSockSendto(&tempAddr, sendFd, 5732, hostName);

	//r är väl connectionID som du randomiserar. Vad hindrar den från att bli samma som någon annan connectionID?
	unsigned int r = 0;
	int i;
	char * msg;
	AcceptedClients * tempClient;
	/*printf("innan switch\n");
	fflush(stdout);
	printf("flag = %d\n", temp->incommingMsg.flag);
	fflush(stdout);
	printf("flag in local = %d\n", incommingMsg->flag);
	fflush(stdout);*/
	switch (incommingMsg->flag)
	{
		//syn received
		case 0:
			printf("case 0\n");
			fflush(stdout);
			msg = "This is an SynAck";
			while(r == 0)
			{
				//printf("rand\n");
				//fflush(stdout);
				r = rand();
			}

			while(1)
			{
				//printf("while\n");
				//fflush(stdout);
				if(pthread_mutex_trylock(&mutex))
				{
					//printf("find client\n");
					//fflush(stdout);
					if(clients == NULL)
					{
						//jag förstår inte riktigt denna if kan vara bra med kommentar till inlämning?
						//Jo nu förstår jag, men kommentar på hela case vore bra
						tempClient = NULL;
					}
					else
					{
						tempClient = findClient(clients->head, temp->remaddr, incommingMsg->id );
					}
	//				printf("client found\n");
	//				fflush(stdout);
					pthread_mutex_unlock(&mutex);
					break;
				}
			}
			if (tempClient != NULL )
			{
	//			printf("tempClient != NULL\n");
	//			fflush(stdout);
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
	//			printf("client == NULL\n");
	//			fflush(stdout);
				createDataHeader(1, r, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg , outgoingMsg);
	//			printf("dataheder\n");
	//			fflush(stdout);
				sent = sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen);
				//error check
				if (!sent)
				{
					printf("Error from 0.2 for msg: %s\n", strerror(errno) );
					fflush(stdout);
				}

	//			printf("sent\n");
	//			fflush(stdout);

				while(1)
				{
					if(pthread_mutex_trylock(&mutex))
					{
	//					printf("add client\n");
	//					fflush(stdout);
						addClient(clients, temp->remaddr, r);
	//					printf("client added\n");
	//					fflush(stdout);
						pthread_mutex_unlock(&mutex);
						break;
					}
				}
			}

			while(1)
			{
				if(pthread_mutex_trylock(&mutex))
				{
	//				printf("find client\n");
	//				fflush(stdout);
	//				printf("blääääää\n");
	//				fflush(stdout);
					tempClient = findClient(clients->head, temp->remaddr, r );
	//				printf("clients found\n");
	//				fflush(stdout);
					pthread_mutex_unlock(&mutex);
					break;
				}
			}
	//		printf("hello client: %d\n", tempClient->id );
	//		fflush(stdout);
	//		printf("win, synAckAck: %d\n", tempClient->synAckAck);
	//		fflush(stdout);
	//		printf("synargs\n");
	//		fflush(stdout);

	//TODO:borde kanske använda memcpy för att det finns en array i sockaddr_in
			synArgs->args = temp;
	//		printf("temp\n");
	//		fflush(stdout);

	//TODO:borde kanske använda memcpy -beror på vad du gör med den.
			synArgs->client = tempClient;
	//		printf("client\n");
	//		fflush(stdout);
			synArgs->win = (int)WINDOWSIZE;
	//		printf("win, synAckAck: %d\n", tempClient->synAckAck);
	//		fflush(stdout);

			while (tempClient->synAckAck != 1)
			{
	//			printf("in synTimer\n");
	//			fflush(stdout);
				//while(clock() < currentTime && tempClient->synAckAck != 1);
				sleep(10);

	//			printf("timer done\n");
	//			fflush(stdout);

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
				//kommentar här? svårt att förstå
				pthread_cancel(tempClient->syn);

				while(1)
				{
					if(pthread_mutex_trylock(&mutex))
					{
						tempClient->timerTime = 2 * (clock() - tempClient->timerTime);
						tempClient->synAckAck = 1;
						fillArrWithSeq0(tempClient);
						//TODO: fråga stina om det verkligen ska var +1 här, var det inte det som Stina ändrade pga mig?
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
				//TODO: why is this checked?
				if(tempClient->synAckAck == 1)
				{
					//recv the correct msg
					if(incommingMsg->seq == tempClient->nextInSeq)
					{
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
						//msg that i do not want or can store
					}
				}
				else
				{
					//resend synAck because have not gotten synAckAck, but client thinks that connection is complete
					//TODO: EHHHHH this can not happen why is this here? This is not in our state-machines
					msg = "This is an synAck";
					createDataHeader(1, r, incommingMsg->seq, WINDOWSIZE, getCRC(strlen(msg), msg), msg, outgoingMsg);
					sent = sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen);
					//error check
					if (!sent)
					{
						printf("Error from case 2.5 for msg: %s\n", strerror(errno) );
						fflush(stdout);
					}
				}
			}
			//NO ELSE? OR PRINT SOMETHING?
			else
			{
				//a client that has not established connection is trying to send a msg
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
					sent = sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen));
					//error check
					if (!sent)
					{
						printf("Error from 3 for msg: %s\n", strerror(errno) );
						fflush(stdout);
					}

					//TODO:borde kanske använda memcpy för att det finns en array i sockaddr_in
					finArgs->args = temp;
					//TODO:borde kanske använda memcpy för att det finns en array dataheader
					finArgs->args->incommingMsg = *incommingMsg;
					//TODO:borde kanske använda memcpy har inte koll på denna struct
					finArgs->client = tempClient;
					finArgs->win = (int )WINDOWSIZE;

					//TODO: Bra med kommentar på vad denna tråd gör?
					int err = pthread_create(&(tempClient->fin), NULL, finTimer, finArgs);
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
			//TODO: borde bara hända om syn är satt och meddelanden scickade
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
	//Här borde det bli fel för att du inte gjort memcpy på tructar med pekare/arrayer
	free(temp);
	free(incommingMsg);
	free(outgoingMsg);
	free(synArgs);
	free(finArgs);
	//free(&sendFd);
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


// void initSockSendto(struct sockaddr_in *myaddr, int fd, int port, char *host)
// {
//     /* bind to an arbitrary return address */
//     /* because this is the client side, we don't care about the address */
//     /* since no application will initiate communication here - it will */
//     /* just send responses */
//     /* INADDR_ANY is the IP address and 0 is the socket */
//     /* htonl converts a long integer (e.g. address) to a network representation */
//     /* htons converts a short integer (e.g. port) to a network representation */
//
//     struct hostent *hp;     /* host information */
//     /* look up the address of the server given its name */
//     hp = gethostbyname(host);
//     if (!hp)
//     {
//     	printf("Error from listenFunc for msg: %s\n", strerror(errno) );
// 			fflush(stdout);
//     }
//
//     memset((char *)myaddr, 0, sizeof(*myaddr));
//     myaddr->sin_family = AF_INET;
//     myaddr->sin_addr.s_addr = htonl(INADDR_ANY);
//     myaddr->sin_port = htons(port);
//     memcpy((void *)&myaddr->sin_addr, hp->h_addr_list[0], hp->h_length);
//
//     // if (bind(fd, (struct sockaddr *)myaddr, sizeof(*myaddr)) < 0)
//     // {
//     //     perror("bind failed");
//     //     exit(EXIT_FAILURE);
//     //
//     // }
// }
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



// void cpyArg(ArgForThreads * to, ArgForThreads * from)
// {
//
// }
