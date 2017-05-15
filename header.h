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

void createDataHeader(int flag, int id, int seq, int windowSize, int crc, char * data , DataHeader * head);

//////////////////////////MsgListOperations////////////////////////////////////////////////////////////////////
typedef struct msgList
{
	clock_t timerStart;
	int sent;							//0 = not sent - 1 = sent
	int acked;						//0 = no ack   - 1 = acked
	DataHeader *data;
	struct msgList *next;
} MsgList;

void createMessages(MsgList *head, int id, int windowSize);


//returns the amount of nodes removed
//int removeIfAcked(ControlStruct c, int windowSize);
//

///////////////////////////////TimerOperations//////////////////////////////////////////////////////////////////////



//TimeOutFunc

//int startTimer (clock_t time);
//void * timerThread (void * arg);
