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
#include "header.h"


#define PORT 5555
#define HOST_NAME_LENGTH 50
#define BUFSIZE 2048


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
void checkMsgAndSendAck (ListHead * head, DataHeader * incommingMsg, DataHeader * outgoingMsg, int fd, struct sockaddr_in remaddr, socklen_t addrlen);
void createDataHeader(int flags, int id, int seq, int windowsize, int crc, char * data, DataHeader * head );


//void * startUpFunc ()
int main(int argc, char *argv[])
{
	DataHeader * incommingMsg;
	ListHead * head = createListHead();
	DataHeader * outgoingMsg;

	//Create socket
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



	while (1)
	{
		int msgRecv = recvfrom(fd, incommingMsg, sizeof(DataHeader), 0, (struct sockaddr *)&remaddr, &addrlen);

		//error check
		if (!msgRecv)
		{
		    printf("Error from listen for syn: %s\n", strerror(errno) );
		    return EXIT_FAILURE;
		}

		else
		{
		    //check data
			switch (incommingMsg->flags)
			{
				//received syn
				case 0:
					 createDataHeader(1, incommingMsg->id, incommingMsg->seq, 3, 0, "This is a SYNACK", outgoingMsg);//creating outgoingMsg
					 checkMsgAndSendAck (head, incommingMsg,outgoingMsg, fd, remaddr, addrlen);
					break;

				//recived ack on syn-ack
				case 1:
					//add msg to list
					//this is the final part of the handshake therefore on ack
					addNodeToList( head, incommingMsg);
					break;

				//received new msg
				case 2:
					createDataHeader(2, incommingMsg->id, incommingMsg->seq, 3, 0, "This is an ACK", outgoingMsg); //creating outgoingMsg
					 checkMsgAndSendAck (head, incommingMsg, outgoingMsg, fd, remaddr, addrlen);
					break;

				// received fin
				case 3:
					createDataHeader(4, incommingMsg->id, incommingMsg->seq, 3, 0, "This is a FINACK", outgoingMsg); //creating outgoingMsg
					 checkMsgAndSendAck (head, incommingMsg, outgoingMsg, fd, remaddr, addrlen);
					break;

				//recived fin ack
				case 4:
					//?
					break;
			}
		}
	}
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
//creates msg
void createDataHeader(int flags, int id, int seq, int windowsize, int crc, char * data , DataHeader * head)
{
	head->flags = flags;
	head->id = id;
	head->seq = seq;
	head->windowsize = windowsize;
	head->crc = crc;
	head->data = data;

}

//check if msg is dup and send ack
void checkMsgAndSendAck (ListHead * head, DataHeader * incommingMsg, DataHeader * outgoingMsg, int fd, struct sockaddr_in remaddr, socklen_t addrlen)
{
	int returnValue;
	if (searchList(head->head, incommingMsg->seq) == -1) // == its not a repeated msg add it to the list
	{
		//add msg to list
		addNodeToList( head, incommingMsg);
	}
	//send ack
	returnValue = sendto(fd, outgoingMsg, sizeof(*outgoingMsg), 0, (struct sockaddr *)&remaddr, &addrlen);

	//error check
	if (!returnValue )
	{
		printf("Error from send ack %s: %s\n", outgoingMsg->data, strerror(errno) );
		exit(EXIT_FAILURE);
	}
	else if (returnValue == -35)
	{
		printf("Something is wrong in the search function");
	}
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
