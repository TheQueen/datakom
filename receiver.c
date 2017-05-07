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


//ListFuncs and struct
typedef struct ListNode ListNode; 

struct ListNode
{
	DataHeader msg; 
	ListNode * next; 
}; 

typedef struct ListHead
{
	ListNode * head; 
}ListHead; 


ListHead * createListHead(); 
ListNode * createListNode(DataHeader * msg);
void addNodeToList(ListHead * head, DataHeader * msg);
void removeAllNodesFromList(ListHead * head); 
int searchList(ListNode * node, int seqNr); // -1 = no 1 = yes


//funcs for socket things
int createSock();
void initSock(struct sockaddr_in *myaddr, int fd, int port);
void checkMsgAndSendAck (ListHead * head, DataHeader * incommingMsg, DataHeader * outgoingMsg, int fd, struct sockaddr_in remaddr, socklen_t addrlen, int * winCounter); 
void fillArrWithSeq0();
int emptyArrIndex();
void addToArr(DataHeader * incommingMsg, int index);
void * listenFunc(void * args);
void * handleMsg (void * args);


//global variables 

DataHeader window[(WINDOWSIZE-1)]; 
ListHead * head;
AccClientListHead * clients; 
int msgThreadCounter; 
int finRecived;

//void * startUpFunc ()
int main(int argc, char *argv[])
{
	//variables
	head = createListHead();
	pthread_t listener; 
	ArgForThreads args; 
	msgThreadCounter = 0; 
	
	fillArrWithSeq0();
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
	finRecived = 0; 
	while (1) 
	{
		if(finRecived && !msgThreadCounter)
		{
	
		}
		else
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
				pthread_create(&msg, NULL, handleMsg, args); 
				msgThreadCounter = msgThreadCounter + 1;
			}
		}
	}
}


void * handleMsg (void * args)
{
	DataHeader * outgoingMsg;
	ArgForThreads * temp = (ArgForThreads*) args; 
	struct sockaddr_in tempAddr = temp->remaddr;
	int returnValue; 
	int r = 0; 
	int i; 
	AcceptedClients * tempClient;
	DataHeader tempMsg;
	
	switch (temp->incommingMsg->flag)
	{
		//syn received
		case 0: 
			while(r == 0)
			{
				r = rand(); 
			}
			createDataHeader(1, r, temp->incommingMsg->seq, 3, temp->incommingMsg->crc, "This is a synAck" , outgoingMsg); 
			sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen); 
			addClient(clients, temp->remaddr, r); 
			break;
		//ack on the synAck recived
		case 1:
			tempClient = findClient(clients->head, temp->remaddr, temp->incommingMsg->id );
			if(tempClient != NULL)
			{
				tempClient->synAckAck = 1;
				tempClient->nextInSeq = temp->incommingMsg->seq; 
			}
			
			break;
		//getMsg from client
		case 2:
			tempClient = findClient(clients->head, temp->remaddr, temp->incommingMsg->id );
			if (tempClient != NULL)
			{
				if(temp->incommingMsg->seq == tempClient->nextInSeq)
				{
					addNodeToList(head, temp->incommingMsg); 
					for(i = 0; i < (WINDOWSIZE-1); i++)
					{
						if (window[i].seq != 0)
						{
							
							addNodeToList(head, &(window[i])); 
						}
					}
					createDataHeader(2,  temp->incommingMsg->id, temp->incommingMsg->seq, 3, temp->incommingMsg->crc, "This is an Ack" , outgoingMsg); 
					sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 
				}
				else if(temp->incommingMsg->seq == (tempClient->nextInSeq+1))
				{
					addToArr(temp->incommingMsg, 0);
					createDataHeader(2,  temp->incommingMsg->id, temp->incommingMsg->seq, 3, temp->incommingMsg->crc, "This is an Ack, I'm missing one msg" , outgoingMsg); 
					sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 
				}
				else if (temp->incommingMsg->seq == (tempClient->nextInSeq+2))
				{
					addToArr(temp->incommingMsg, 1);
					createDataHeader(2,  temp->incommingMsg->id, temp->incommingMsg->seq, 3, temp->incommingMsg->crc, "This is an Ack, not accept any more" , outgoingMsg); 
					sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 
				}
				else if (temp->incommingMsg->seq == (tempClient->nextInSeq-1))
				{
					createDataHeader(2,  temp->incommingMsg->id, temp->incommingMsg->seq, 3, temp->incommingMsg->crc, "This is an Ack, i alredy have this one" , outgoingMsg); 
					sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 
				}
				else
				{
					//msg that i do not want
				}
			}
			else
			{
				//a client that have not established connection is trying to send a msg
			}
			break;
		//received fin
		case 3://Jag e osäker på dessa hur vi ska ha dem fungera. just nu skickar jag en fin från server efter en sec
			createDataHeader(3,  temp->incommingMsg->id, temp->incommingMsg->seq, 3, temp->incommingMsg->crc, "This is an FinAck " , outgoingMsg); 
			sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) (temp->addrlen)); 
			
			sleep(1); 
			createDataHeader(4,  temp->incommingMsg->id, temp->incommingMsg->seq, 3, temp->incommingMsg->crc, "This is an Fin " , outgoingMsg); 
		break;
		
		case 4:
			//received fin ack
			removeClient(clients,tempAddr,temp->incommingMsg->id);
			break;
	}
	msgThreadCounter = msgThreadCounter - 1;
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

//check if msg is dup and send ack
void checkMsgAndSendAck (ListHead * head, DataHeader * incommingMsg, DataHeader * outgoingMsg, int fd, struct sockaddr_in remaddr, socklen_t addrlen, int * winCounter)
{
	int returnValue; 
	int searchReturn = searchList(head->head, incommingMsg->seq);
	if ( searchReturn == -1) // == its not a repeated msg add it to the list
	{
		//add msg to list
		addNodeToList( head, incommingMsg);
		switch (*winCounter)
		{
				//Här har ja problem
			case 1:
				//om de låg på correkt index så denna på 0 så skulle det bara vara att plocka dem från arrayen
				//nu måste vi köra forloop :/
				break;
			case 2:
				break;
			case 3:
				break; 
			default:
				break;
				
		}
	}
	else if (searchReturn == -35)
	{
		printf("Something is wrong in the search function");
	}
	
	//send ack
	returnValue = sendto(fd, outgoingMsg, sizeof(*outgoingMsg), 0, (struct sockaddr *)&remaddr, addrlen); 	
	
	//error check
	if (!returnValue )
	{
		printf("Error from send ack %s: %s\n", outgoingMsg->data, strerror(errno) ); 
		exit(EXIT_FAILURE); 
	}
	
}

//to fill arr with "Empty" dataHeaders
void fillArrWithSeq0()
{
	int i = 0; 
	for (i = 0; i<(WINDOWSIZE-1);i++)
	{
		window[i].seq = 0;
	}
}

//find an empty index
int emptyArrIndex()
{
	int i = 0;
	for(i = 0; i < (WINDOWSIZE-1); i++)
	{
		if(window[i].seq == 0)
		{
			return i; 
		}
	}
	exit(EXIT_FAILURE);
}

//
void addToArr(DataHeader * incommingMsg, int index)
{
	window[index] = *incommingMsg; 
	strcpy(window[index].data, incommingMsg->data);
}



//ListFuncs

ListHead * createListHead()
{
	ListHead * head; 
	if ((head = (ListHead *)malloc(sizeof(ListHead))) == NULL) 
	{
		printf("Cant malloc ListHead: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	head->head = NULL; 
	return head; 
}

ListNode * createListNode(DataHeader * msg)
{
	ListNode * node; 
	if((node = (ListNode * )malloc(sizeof(ListNode)))== NULL) 
	{
		printf("Cant malloc node: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	node->msg = *msg;
	strcpy(node->msg.data, msg->data);
	node->next = NULL; 
	return node; 
}

void addNodeToList(ListHead * head, DataHeader * msg)
{
	ListNode * temp = head->head;
	head->head = createListNode(msg);
	head->head->next = temp; 
	
}





void removeAllNodesFromList(ListHead * head)
{
	while (head->head != NULL)
	{
		ListNode * temp = head->head->next; 
		free(head->head);
		head->head = temp;
		removeAllNodesFromList(head); 
	}
}

int searchList(ListNode * node, int seqNr)
{
	if(node == NULL)
	{
		return -1; 
	}
	else if (node->msg.seq == seqNr)
	{
		return 1; 
	}
	else 
	{
		searchList(node->next, seqNr);
	}
	return -35; 
}

