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

//traansport header

typedef  struct header 
{
	int flag;
	int id; 
	int seq; 
	int windowsize; 
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

//struct with accepted clients
typedef struct AcceptedClients AcceptedClients;

struct AcceptedClients
{
	struct sockaddr_in remaddr; 
	int id; 
	int synAckAck; 
	int nextInSeq; /*next seqNr that is expekted*/
	AcceptedClients * next;
};

typedef struct AccClientListHead
{
	AcceptedClients * head;
}AccClientListHead; 

//create transportHeader
void createDataHeader(int flag, int id, int seq, int windowsize, int crc, char * data , DataHeader * head); 

//list funcs for 
void addClient(AccClientListHead * head, struct sockaddr_in remaddr, int id ); 
AcceptedClients * findClient(AcceptedClients * client, struct sockaddr_in remaddr, int id );
int removeClient(AccClientListHead * clientHead, struct sockaddr_in remaddr, int id);

//TimeOutFunc

int startTimer (clock_t time); 
void * timerThread (void * arg);