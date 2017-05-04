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