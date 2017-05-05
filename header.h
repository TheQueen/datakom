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

//treansport header

typedef  struct header 
{
	int flags;
	int id; 
	int seq; 
	int windowsize; 
	int crc; 
	char * data; 
} DataHeader; 

typedef struct argumetForThreads
{
	int fd; /*socket file descriptor*/
    struct sockaddr_in sock; /*udp socket*/
    struct sockaddr_in remaddr;     /* remote address */
    socklen_t addrlen; /* length of addresses */
	DataHeader * incommingMsg; /*buffer for incomming data*/
	int nextInSeq; /*next seqNr that is expekted*/
}ArgForThreads; 


void createDataHeader(int flags, int id, int seq, int windowsize, int crc, char * data , DataHeader * head); 

//TimeOutFunc

int startTimer (clock_t time); 
void * timerThread (void * arg);