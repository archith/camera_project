#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "ra_globals.h"

#define BUFFER_SIZE						(2048*1)
#define MAX_NAME_LENGTH					64
#define CREATE_PARAMETER_TABLE_LENGTH	2

#define START_NUMBER_TO_BIND			1024

#define PARAMETER_ACCEPT				"accept"
#define PARAMETER_MAXPACKET				"maxPacket"

#define ARGUMENT_ACCEPT_G711A			"G.711A"
#define ARGUMENT_ACCEPT_G711U			"G.711U"

#define PAYLOAD_TYPE_PCMA	0
#define PAYLOAD_TYPE_PCMU	1

// main routine to bind a port for the input rtp audio
int create_rtp_audio(char *para);

// convert the string to the value by the parameter name
int get_value_from_string(char *para, char *target, char *result);

// parse the input arguments
int parse_create_command(char * para);

// try to bind the designated port
int bind_socket(int port);

// main loop to wait the receive the rtp audio data
void recv_audio_data();

// copy the rtp header
void memcpy_rtp_header(RTP_header * hdr, char * buf);

// search the SSRC identifier
int find_ssrc_identifier(char *buf, int len, RTP_header * hdr);

// get the packet sequence number
unsigned short get_sequence_number(char c1, char c2);

