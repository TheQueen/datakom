#include <time.h>

//treansport header

typedef  struct header
{
	int flag;
	int id;
	int seq;
	int windowSize;
	int crc;
	char data[50];
} DataHeader;

void createDataHeader(int flag, int id, int seq, int windowsize, int crc, char * data , DataHeader * head);

//TimeOutFunc

//int startTimer (clock_t time);
//void * timerThread (void * arg);
