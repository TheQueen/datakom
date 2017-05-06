#include "header.h"

#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


void createDataHeader(int flag, int id, int seq, int windowSize, int crc, char * data , DataHeader * head)
{
	head->flag = flag;
	head->id = id;
	head->seq = seq;
	head->windowSize = windowSize;
	head->crc = crc;
	int i; 
	for ( i = 0; data[i] != '\0'; i++)
	{
		head->data[i] = data[i];
	}
	head->data[i] = '\0';

}

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
