#include "header.h"
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>

void createDataHeader(int flag, int id, int seq, int windowsize, int crc, char * data , DataHeader * head)
{
	head->flag = flag;
	head->id = id;
	head->seq = seq;
	head->windowsize = windowsize;
	head->crc = crc;
	strcpy(head->data, data); 
}

void addClient(AccClientListHead * head, struct sockaddr_in remaddr, int id )
{
	AcceptedClients * temp;
	if(head->head == NULL)
	{
		head->head->remaddr = remaddr; 
		head->head->id = id; 
		head->head->synAckAck = 0; 
	}
	else
	{
		temp = head->head; 
		head->head->remaddr = remaddr; 
		head->head->id = id; 
		head->head->next = temp;
		head->head->synAckAck = 0; 
	}
}

AcceptedClients * findClient(AcceptedClients * client, struct sockaddr_in remaddr, int id )
{
	if(client == NULL)
	{
		return NULL; 
	}
	if(client->remaddr.sin_addr.s_addr == remaddr.sin_addr.s_addr && client->id == id)
	{
		return client; 
	}
	else
	{
		findClient( client->next, remaddr, id ); 
	}
	return NULL;
}

AcceptedClients * findClientBefore(AcceptedClients * client, struct sockaddr_in remaddr, int id)
{
	if(client == NULL)
	{
		return NULL; 
	}
	else if (client->next->remaddr.sin_addr.s_addr == remaddr.sin_addr.s_addr && client->next->id == id)
	{
		return client; 
	}
	else
	{
		findClientBefore(client->next, remaddr,id); 
	}
	return NULL; 
}

int removeClient(AccClientListHead * clientHead, struct sockaddr_in remaddr, int id)
{
	AcceptedClients * clientToRemove = findClient(clientHead->head, remaddr, id ); 
	if(clientToRemove == clientHead->head)
	{
		clientHead->head = clientToRemove->next; 
		clientToRemove->next = NULL; 
		free(clientToRemove);
		return 1; 
	}
	AcceptedClients * clientbefore = findClientBefore(clientHead->head, remaddr, id );
	if (clientbefore == NULL)
	{
		return -1; 
	}
	else 
	{
		clientbefore->next = clientToRemove->next; 
		clientToRemove->next = NULL; 
		free(clientToRemove);
		return 1; 
	}
	return -1; 
	
}

//timerfuncs

int startTimer (clock_t time)
{
	pthread_t pid; 
	int returnValue = 0; 
	
	returnValue = pthread_create(&pid, NULL, timerThread, (void *) time);
	
	if (returnValue)
	{
		printf("Error from listen for syn: %s\n", strerror(errno) ); 
		return -1; 
	}
	return 1; 
}

void* timerThread (void * arg)
{
	clock_t * time = (clock_t *)arg;
	clock_t currentTime = clock() + *time; 
	
	while (clock() < currentTime);
	return 0;
}

//get roundtrip time
/*

start time when send away syn
clock_t start = clock(); 

when receive synAck
clock_t end = clock(); 

clock_t roundTripTime = 2*(end - start); 



*/