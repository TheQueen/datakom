#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

void createDataHeader(int flag, int id, int seq, int windowSize, int crc, char * data , DataHeader * head);

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

unsigned short getCRC (int msgSize, char * msg);
unsigned short calcError (unsigned short crc, int msgSize, char * msg);
