#include "header.h"

void createDataHeader(int flag, int id, int seq, int windowSize, int crc, char * data , DataHeader * head)
{
	head->flag = flag;
	head->id = id;
	head->seq = seq; 
	head->windowSize = windowSize;
	head->crc = crc;
	strcpy(head->data, data); 
}

//AcceptedClientlist
void addClient(AccClientListHead * head, struct sockaddr_in remaddr, int id )
{
	printf("Mwoa\n");
	fflush(stdout);
	AcceptedClients * temp;
	
	AcceptedClients * toAdd;
	if(head == NULL)
	{
		head = (AccClientListHead*)malloc(sizeof(AccClientListHead));
		head->head = (AcceptedClients*) malloc(sizeof(AcceptedClients));
		head->head->remaddr = remaddr; 
		head->head->id = id; 
		head->head->timerTime = clock(); 
		head->head->msgs = NULL; 
		head->head->synAckAck = 0;
		head->head->finAck = 0;
		head->head->syn = (pthread_t *)malloc(sizeof(pthread_t));
		pthread_mutex_init(&(head->head->mutex),NULL); 
	}
  
	else
	{
		toAdd = (AcceptedClients*) malloc(sizeof(AcceptedClients));
		toAdd->remaddr = remaddr; 
		toAdd->id = id; 
		toAdd->timerTime = clock(); 
		toAdd->msgs = NULL; 
		toAdd->next = head->head; 
		toAdd->synAckAck = 0;
		toAdd->finAck = 0;
		pthread_mutex_init(&(toAdd->mutex),NULL); 
		head->head = toAdd; 
		
	}
	printf("done\n");
	fflush(stdout);
}

AcceptedClients * findClient(AcceptedClients * client, struct sockaddr_in remaddr, int id )
{
	printf("huh?\n");
	fflush(stdout);
	if(client == NULL)
	{
		printf("client null\n");
		fflush(stdout);
		return NULL; 
	}
	if(client->remaddr.sin_addr.s_addr == remaddr.sin_addr.s_addr && client->id == id)
	{
		printf("client found\n");
		fflush(stdout);
		return client; 
	}
	else
	{
		printf("you turn me right round bby right round\n");
		fflush(stdout);
		return findClient( client->next, remaddr, id ); 
	}
	printf("oh nooooooo\n");
	fflush(stdout);
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
		return findClientBefore(client->next, remaddr,id); 
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

void addMsgToClient(AcceptedClients * client, DataHeader * msg)
{
	ClientMsgList * temp; 
	if (client->msgs == NULL)
	{
		client->msgs->seq = msg->seq;
		strcpy(client->msgs->data, msg->data);
		client->msgs->next = NULL; 
		client->msgs->last = 1;
		client->msgs->prev = NULL; 
		
	}
	else 
	{
		temp = client->msgs; 
		client->msgs->seq = msg->seq;
		strcpy(client->msgs->data, msg->data);
		temp->prev = client->msgs; 
		client->msgs->next = temp;
		client->msgs->last = 0; 
	}
}

ClientMsgList * findTheFirstMsg(ClientMsgList * msg)
{
	if(msg == NULL)
	{
		return NULL; 
	}
	else if (msg->last == 1)
	{
		return msg; 
	}
	else 
	{
		return findTheFirstMsg(msg->next); 
	}
	return NULL;
}

void printMsg(ClientMsgList * firstMsg)//firstMsg = client->msgs
{
	ClientMsgList * msg = findTheFirstMsg(firstMsg);
	int seq = msg->seq; 
	
	printf("------- Message Recived -------\n");
	printf ("\n%s ", msg->data); 
	
	while(msg != NULL)
	{
		
		msg = getMsgToPrint(firstMsg, seq); 
		seq = msg->seq; 
		printf ("%s ", msg->data); 
		
	}

}

void removeMsg(ClientMsgList * msg)
{
	if(msg->next != NULL)
	{
		msg->next->prev = msg->prev; 	
	}
	
	if(msg->prev != NULL)
	{
		msg->prev->next = msg->next; 
	}
	
	msg->next = NULL; 
	msg->prev = NULL; 
}

ClientMsgList * getMsgToPrint (ClientMsgList * msg, int seq)
{
	if(msg->seq == (seq+1))
	{
		return msg; 
	}
	else
	{
		return getMsgToPrint(msg->next, seq);
	}
	return NULL;
}



//ListFuncs

ListHead * createListHead()
{
	ListHead * head; 
	if ((head = (ListHead *)malloc(sizeof(ListHead))) == NULL) 
	{
		printf("Cant malloc ListHead: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	head->head = NULL; 
	return head; 
}

ListNode * createListNode(DataHeader * msg)
{
	ListNode * node; 
	if((node = (ListNode * )malloc(sizeof(ListNode)))== NULL) 
	{
		printf("Cant malloc node: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	node->msg.flag = msg->flag; 
	node->msg.id = msg->id; 
	node->msg.seq = msg->seq; 
	node->msg.windowSize = msg->windowSize; 
	node->msg.crc = msg->crc; 
	strcpy(node->msg.data, msg->data);
	node->next = NULL; 
	return node; 
}

void addNodeToList(ListHead * head, DataHeader * msg)
{
	ListNode * temp = head->head;
	head->head = createListNode(msg);
	head->head->next = temp; 
	
}

void removeAllNodesFromList(ListHead * head)
{
	while (head->head != NULL)
	{
		ListNode * temp = head->head->next; 
		free(head->head);
		head->head = temp;
		removeAllNodesFromList(head); 
	}
}

int searchList(ListNode * node, int seqNr)
{
	if(node == NULL)
	{
		return -1; 
	}
	else if (node->msg.seq == seqNr)
	{
		return 1; 
	}
	else 
	{
		searchList(node->next, seqNr);
	}
	return -35;
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

void * finTimer(void * arg)
{
	int type = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if (type != 0)
	{
		printf("Error in type");
	}
	
	DataHeader * outgoingMsg = NULL;
	char * msg; 
	FinArg * finArgs = arg; 
	ArgForThreads * temp = finArgs->args;
	AcceptedClients * tempClient = finArgs->client;
	struct sockaddr_in tempAddr = temp->remaddr;
	
	clock_t time = (clock_t )tempClient->timerTime;
	clock_t currentTime = clock() + time; 
	
	while(1)
	{
		while (clock() < currentTime);
		
		if(tempClient->finAck == 1)
		{
			break; 
		}
		
		msg = "This is an Fin "; 
		createDataHeader(4,  temp->incommingMsg.id, temp->incommingMsg.seq, finArgs->win, getCRC(strlen(msg), msg), msg, outgoingMsg); 
		sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen); 
	}
	return (void *) 1; 
}

void * synTimer(void * arg)
{
	printf("plehe");
	fflush(stdout); 
	int type = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if (type != 0)
	{
		printf("Error in type");
	}
	printf("plehadse");
	fflush(stdout);
	FinArg * finArgs = arg;
	printf("plehetqfea");
	fflush(stdout);
	DataHeader * outgoingMsg = (DataHeader*)malloc(sizeof(DataHeader));
	printf("plehsdadase");
	fflush(stdout);
	ArgForThreads * temp = finArgs->args;
	AcceptedClients * tempClient = finArgs->client;
	struct sockaddr_in tempAddr = temp->remaddr;
	
	clock_t time = (clock_t )tempClient->timerTime;
	clock_t currentTime = clock() + time; 
	
	while (1)
	{
		while(clock() < currentTime);
		
		if(tempClient->synAckAck == 1)
		{
			break; 
		}
		else
		{
			char * msg = "This is an SynAck"; 
			createDataHeader(1, temp->incommingMsg.id, temp->incommingMsg.seq, finArgs->win, temp->incommingMsg.crc, msg , outgoingMsg); 
			sendto(temp->fd, outgoingMsg, sizeof(*outgoingMsg), 0 ,  (struct sockaddr *)&tempAddr, (socklen_t) temp->addrlen); 
		}
	}
	return (void *) 1; 
}

///////////////////////////////ErrorChecking//////////////////////////////////////////////////////////////////////



//convert char to bit
void convertChar (int * bitArr, int arrSize, char * msg)
{
	int i, j; 
	unsigned short k; 
	k = msg[0]; 
	printf("k = %d\n", k);
	printf("bitArr = ");
	for (j = 0; j < arrSize; j++)
	{
		for (i = 7; i >= 0; i--)
		{
			bitArr[i] = (msg[j] >> i) & 1;
			printf("%d", bitArr[i]);
		}
		printf(" ");
	}
	printf("\n");
}

unsigned short getCRC (int msgSize, char * msg)
{
	int i, bitIndex, testBit, xorFlag = 0; 
	unsigned short rest = 0xff, chOfMsg; 
	
	for (i = 0; i < msgSize; i++)
	{
		chOfMsg = msg[i]; 
		testBit = 0x80;
		
		printf("ch = %d ", chOfMsg);
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
	printf("CRC from client %d\n", crc);
	printf("msgSize: %d\n", msgSize);
	unsigned short rest; 
	rest = getCRC(msgSize, msg); 
	rest = rest - crc; 
	printf("\n final rest in Error = %d\n\n", rest);
	return rest;
}	

