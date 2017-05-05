#include <time.h>

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

void createDataHeader(int flags, int id, int seq, int windowsize, int crc, char * data , DataHeader * head); 

//TimeOutFunc

int startTimer (clock_t time); 
void * timerThread (void * arg);