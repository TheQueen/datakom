#include <time.h>
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
#include <time.h>

#define POLY 0x1021

//transport header
typedef  struct header
{
	int flag;
	int id; 
	int seq; 
	int windowSize; 
	unsigned short crc;
	char data[50]; 
} DataHeader; 


//struct for passing arguments in threads
typedef struct argumetForThreads
{
	int fd; /*socket file descriptor*/
    struct sockaddr_in sock; /*udp socket*/
    struct sockaddr_in remaddr;     /* remote address */
    socklen_t addrlen; /* length of addresses */
	DataHeader incommingMsg; /*buffer for incomming data*/
	
}ArgForThreads; 


//msg list fro the accepted clients conteins the seqNr for msg and the msg
typedef struct ClientMsgList ClientMsgList;

struct ClientMsgList
{
	int seq; 
	char data[50]; 
	ClientMsgList * next;
	int last; 
	ClientMsgList * prev; 
}; 

//struct with accepted clients
typedef struct AcceptedClients AcceptedClients;

struct AcceptedClients
{
	struct sockaddr_in remaddr; 
	int id; 
	int synAckAck; //if 1 connection asteblished 
	int finAck; 
	int nextInSeq; /*next seqNr that is expekted*/
	ClientMsgList * msgs; //msgs from the client
	clock_t timerTime; //time to wait until finAck
	pthread_t syn; // pthread for synTimer
	pthread_t fin; // pthread for finTimer
	DataHeader window[2]; // BYT TILL LISTA? 
	pthread_mutex_t mutex; //mutex 
	
	
	AcceptedClients * next;
};

typedef struct AccClientListHead
{
	AcceptedClients * head;
}AccClientListHead; 

//finThreadStruct
typedef struct FinArg
{
	AcceptedClients * client; 
	ArgForThreads * args; 
	int win; 
	
}FinArg;

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

//create transportHeader
void createDataHeader(int flag, int id, int seq, int windowSize, int crc, char * data , DataHeader * head); 

//list funcs for Accseptet clients in server
void addClient(AccClientListHead * head, struct sockaddr_in remaddr, int id ); 
AcceptedClients * findClient(AcceptedClients * client, struct sockaddr_in remaddr, int id );
int removeClient(AccClientListHead * clientHead, struct sockaddr_in remaddr, int id);
void addMsgToClient(AcceptedClients * client, DataHeader * msg);//saves a msg sent by the client in the saved acceptedclient struct/list
ClientMsgList * findTheFirstMsg(ClientMsgList * msg); 
void printMsg(ClientMsgList * firstMsg); 
void removeMsg(ClientMsgList * msg);
ClientMsgList * getMsgToPrint (ClientMsgList * msg, int seq);


//////////////////////////MsgListOperations////////////////////////////////////////////////////////////////////
typedef struct msgList
{
	pthread_t thread;
	int sent;							//0 = no ack   - 1 = acked
	int acked;						//0 = no ack   - 1 = acked
	DataHeader *data;
	struct msgList *next;
} MsgList;

void createMessages(MsgList *head, int id, int seqStart, int windowSize);

void setAck(MsgList * head, int seq, int windowSize);

MsgList *removeFirstUntilNotAcked(MsgList *head, int *sendPermission);

///////////////////////////////ErrorCheck//////////////////////////////////////////////////////////////////////


int startTimer (clock_t time); 
void * timerThread (void * arg);
void * finTimer(void * arg); 
void * synTimer(void * arg);

//convert char to bit
void convertChar (int * bitArr, int arrSize, char * msg);
unsigned short getCRC (int msgSize, char * msg); 
unsigned short calcError (unsigned short crc, int msgSize, char * msg);

