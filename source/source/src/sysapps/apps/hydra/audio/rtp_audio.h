#include <arpa/inet.h>

#include "MESS.h"
#include "audio_shm.h"

#define RTP_MAX_HEADER_SIZE		sizeof(RTP_header)
#define RTP_MAX_DATA_SIZE		(8192*10)
#define RTP_MAX_PACKET_SIZE		(RTP_MAX_HEADER_SIZE + RTP_MAX_DATA_SIZE)


typedef struct
{
	char data[RTP_MAX_PACKET_SIZE];
	int len;
} AudioRTP_Buffer;

typedef struct
{
	char data[RTP_MAX_DATA_SIZE];
	int len;
	int written_len;
} G711_Buffer;


void open_client_socket();
void process_rtp_download(char *data, int len, int type);
void close_client_socket();
void generate_random_ssrc();
void init_rtp_sender();

