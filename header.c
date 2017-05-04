#include "header.h"

void createDataHeader(int flags, int id, int seq, int windowsize, int crc, char * data , DataHeader * head)
{
	head->flags = flags;
	head->id = id;
	head->seq = seq;
	head->windowsize = windowsize;
	head->crc = crc;
	head->data = data;
}
