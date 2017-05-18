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





//transport header

typedef  struct header 
{
	int flag;
	int id; 
	int seq; 
	int windowSize; 
	int crc; 
	char data[50]; 
} DataHeader; 


//struct for passing arguments in threads
typedef struct argumetForThreads
{
	int fd; /*socket file descriptor*/
    struct sockaddr_in sock; /*udp socket*/
    struct sockaddr_in remaddr;     /* remote address */
    socklen_t addrlen; /* length of addresses */
	DataHeader * incommingMsg; /*buffer for incomming data*/
	
}ArgForThreads; 


//msg list fro the accepted clients conteins the seqNr for msg and the msg
typedef struct msgList msgList;

struct msgList
{
	int seq; 
	char data[50]; 
	msgList * next;
	int last; 
	msgList * prev; 
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
	msgList * msgs; //msgs from the client
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
msgList * findTheFirstMsg(msgList * msg); 
void printMsg(msgList * firstMsg); 
void removeMsg(msgList * msg);
msgList * getMsgToPrint (msgList * msg, int seq);


//TimeOutFunc

int startTimer (clock_t time); 
void * timerThread (void * arg);
void * finTimer(void * arg); 
void * synTimer(void * arg);

//convert char to bit
void convertChar (int * bitArr, int arrSize, char * msg);
unsigned short getCRC (int msgSize, char * msg); 
unsigned short calcError (unsigned short crc, int msgSize, char * msg);