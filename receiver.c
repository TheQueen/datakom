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
void addToArr(DataHeader * incommingMsg, int * winCounter);
void * listenFunc(void * args);
void * handleMsg (void * args);

//global variables 

DataHeader window[(WINDOWSIZE-1)]; 
ListHead * head;

//void * startUpFunc ()
int main(int argc, char *argv[])
{
	//variables
	head = createListHead();
	int winCounter = 0; // behövs inte?  
	pthread_t listener; 
	ArgForThreads args; 
	
	fillArrWithSeq0();
    
	//Create socket
	
    args.addrlen = sizeof(args.remaddr);           
    args.fd = createSock();
    initSock(&(args.sock), args.fd, PORT);

    printf("Socket created and initiated!\n");
	
	
	
	thread_create(&listener, NULL, listenFunc, &args); 
	
	return 0; 		    
}


void * listenFunc(void * args)
{
	ArgForThreads * temp = (ArgForThreads*) args; 
	struct sockaddr_in tempAddr = temp->remaddr;
	pthread_t msg; 
	while (1) 
	{
		int msgRecv = recvfrom(temp->fd, temp->incommingMsg, sizeof(DataHeader), 0, (struct sockaddr *)&tempAddr, (socklen_t*)&(temp->addrlen));
		
		//error check
		if (!msgRecv)
		{
		    printf("Error from listenFunc for msg: %s\n", strerror(errno) ); 
		   
		}
		
		else 
		{ 
			thread_create(&msg, NULL, handleMsg, args); 
		}
	}
}


void * handleMsg (void * args)
{
	
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
	returnValue = sendto(fd, outgoingMsg, sizeof(*outgoingMsg), 0, (struct sockaddr *)&remaddr, &addrlen); 	
	
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
	for (i = 0; i<WINDOWSIZE;i++)
	{
		window[i].seq = 0;
	}
}

//find an empty index
int emptyArrIndex()
{
	int i = 0;
	for(i = 0; i < WINDOWSIZE; i++)
	{
		if(window[i].seq == 0)
		{
			return i; 
		}
	}
	exit(EXIT_FAILURE);
}

//
void addToArr(DataHeader * incommingMsg, int * winCounter)
{
	int index = emptyArrIndex();
	window[index] = *incommingMsg; 
	strcpy(window[index].data, incommingMsg->data);
	*winCounter = (*winCounter) + 1;
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