#include "header.h"

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
void setAck(MsgList * head, int seq, int windowSize)
{
	int i;

	for(i = 0; i<windowSize; i++)
	{
		//TODO: mutex stuff
		if(head->data->seq == seq && head->sent)
		{
			pthread_cancel(head->thread);
			head->acked = 1;
			break;
		}
		else if(head->next != NULL)
		{
			head = head->next;
		}
	}
}

void createMessages(MsgList *head, int id, int seqStart, int windowSize)
{
	int msgLength = 0;
	int i;
	char str[10];
	MsgList *node = head;

	printf("Enter number of packages to send to receiver: ");
	scanf("%d", &msgLength);
	printf("\n");

	for (i=seqStart; i<msgLength+seqStart; i++)
	{
		node =(MsgList*)malloc(sizeof(MsgList));
		node->sent = 0;
		node->acked = 0;
		node->data = (DataHeader*)malloc(sizeof(DataHeader));
		snprintf(str, sizeof(str), "%d", i);//just helps to set the message to the number of the message
		createDataHeader(2, id, i, windowSize, getCRC(sizeof(str), str), str, node->data);
		node->next = NULL;
		node = node->next;
	}
}

MsgList *removeFirstUntilNotAcked(MsgList *head, int *sendPermission)
{
	MsgList *node = head;

	while(node != NULL)
	{
		if(node->acked)
		{
			head = head->next;
			free(node->data);
			node->data = NULL;
			free(node);
			node = head;
			*sendPermission = *sendPermission - 1;
		}
		else
		{
			break;
		}
	}
	return head;
}

///////////////////////////////ErrorChecking//////////////////////////////////////////////////////////////////////

unsigned short getCRC (int msgSize, char * msg)
{
	int i, bitIndex, testBit, xorFlag = 0; 
	unsigned short rest = 0xff, chOfMsg;


	printf("All the rest\n");

	for (i = 0; i < msgSize; i++)
	{
		chOfMsg = msg[i];
		printf("ch = %d ", chOfMsg);
		testBit = 0x80;

		for(bitIndex = 0; bitIndex < 8; bitIndex++)
		{
			//Checks if first bit is 1 if it is then a xor should be done
			if (rest & 0x80)
			{
				xorFlag = 1;
			}
			else
			{
				xorFlag = 0;
			}

			rest = rest << 1; //move and add a 0

			if (chOfMsg & testBit) //if the bit is 1 add 1 to rest (replaces the 0 above with a 1)
			{
				rest = rest + 1;
			}

			if (xorFlag) // makes the divide if the checked bit is 1
			{
				rest = rest ^ POLY;
			}

			testBit = testBit >> 1; //move so that it checks the next bit
			printf("%d ", rest);
		}
		printf("\n");
	}
	printf("\n final rest = %d\n\n", rest);
	return rest;
}

unsigned short calcError (unsigned short crc, int msgSize, char * msg)
{
	unsigned short rest;
	rest = getCRC(msgSize, msg);
	rest = rest - crc;
	printf("\n final rest in Error = %d\n\n", rest);
	return rest;
}
