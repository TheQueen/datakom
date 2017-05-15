#include "header.h"

#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


void createDataHeader(int flag, int id, int seq, int windowSize, int crc, char * data , DataHeader * head)
{
	printf("In createDataHeader\n");
	fflush(stdout);
	head->flag = flag;
	head->id = id;
	head->seq = seq;
	head->windowSize = windowSize;
	head->crc = crc;
	strcpy(head->data, data);
	printf("End of createDataHeader\n");
	fflush(stdout);
}
//////////////////////////MsgListOperations////////////////////////////////////////////////////////////////////
void createMessages(MsgList *head, int id, int windowSize)
{
	int msgLength = 0;
	int i;
	//int random = 13; //TODO:Need to randomize num for sequece? -ask Stina
	char str[10];
	MsgList *node = head;

	printf("Enter number of packages to send to receiver: ");
	scanf("%d", &msgLength);
	printf("\n");

	for (i=0; i<msgLength; i++)
	{
		node =(MsgList*)malloc(sizeof(MsgList));
		node->timerStart = 0; //TODO: learn timer stuff
		node->sent = 0;
		node->acked = 0;
		node->data = (DataHeader*)malloc(sizeof(DataHeader));
		snprintf(str, sizeof(str), "%d", i);//just helps to set the message to the number of the message
		createDataHeader(2, id,  /*?random+*/i, windowSize, /*insert real crc-value*/0, str, node->data);
		node->next = NULL;
		node = node->next;
	}
}

///////////////////////////////TimerOperations//////////////////////////////////////////////////////////////////////

//
// //timerfuncs
//
// int startTimer (clock_t time)
// {
// 	pthread_t pid;
// 	int returnValue = 0;
//
// 	returnValue = pthread_create(&pid, NULL, timerThread, (void *) time);
//
// 	if (returnValue)
// 	{
// 		printf("Error from listen for syn: %s\n", strerror(errno) );
// 		return -1;
// 	}
// 	return 1;
// }
//
// void* timerThread (void * arg)
// {
// 	clock_t * time = (clock_t *)arg;
// 	clock_t currentTime = clock() + *time;
//
// 	while (clock() < currentTime);
//
// }
//
// //get roundtrip time
// /*
//
// start time when send away syn
// clock_t start = clock();
//
// when receive synAck
// clock_t end = clock();
//
// clock_t roundTripTime = end - start;
//
//
//
// */
//
