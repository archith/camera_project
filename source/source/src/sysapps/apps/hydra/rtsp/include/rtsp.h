#ifndef _RESPONSE_H
#define _RESPONSE_H

#include <netinet/in.h>

typedef struct
{
	/* network */
	int listnum;		// fd in connection list
	struct sockaddr_in cli_vsin;	// socket address of client, video
	struct sockaddr_in cli_asin;	// socket address of client, audio
	struct sockaddr_in srv_vsin;	// socket address of server, video
	struct sockaddr_in srv_asin;	// socket address of server, audio

	struct sockaddr_in rtcp_cli_vsin;	// socket address of client, video
	struct sockaddr_in rtcp_cli_asin;	// socket address of client, audio
	struct sockaddr_in rtcp_srv_vsin;	// socket address of server, video
	struct sockaddr_in rtcp_srv_asin;	// socket address of server, audio

	/* rtsp protocol */
	char method[16];
	char requri[64];
	char rtspver[16];
	unsigned short user_agent;	/* see below */
	unsigned short cseq;
	unsigned short bandwidth;

#define ACCESS_DENY	0
#define ACCESS_GRANTED	1
	char is_multicast:1, is_overtcp:1,	// RTP over RTSP ?
	  is_overhttp:1,
	  has_video:1,
	  has_audio:1,
	  is_mobile:1,
	  is_audio:1,	// only valid for a request period
	  access_granted:1;

	/* security info */
	char username[32];
	char password[32];
	char xsessioncookie[32];	// only need for RTP/RTSP/HTTP condition
	/* connection status */
	int status;
#define RTSP_CONN_EMPTY		0
#define RTSP_CONN_ESTABLISHING	1
#define RTSP_CONN_CMDPARSING	2
#define RTSP_CONN_CMDPARSED	3
#define RTSP_CONN_ESTABLISHED	4
#define RTSP_CONN_SENDING	5
#define RTSP_CONN_DISCONNECTING	6


	/* rtp information */
	int rtpvinfoidx;
	int rtpainfoidx;
	
	int sendtype;
	int video_type;
	/* tmp buff */
	char buff[1024];
	int bufsize;
} rtsp_req_t;

/* 
 user agent definition:
 There are lots of user agent which can support RTSP and RTP, unfortunatly,
 the behavior of each user agent differs which increase the difficulty of our
 implementation, therefore, it is a necessary sin to use this trick to play with
 all these user agents.
*/

#define Sercomm		0
#define Realone		1
#define Quicktime	2
#define Mplayer		3
#define VLC		4
#define Axis		5
#define pvPlayer	6
#define Others		7

/* RTSP default port number */
//#define RTSP_PORT	554

/* response.c */
int rtsp_process (int index);

int rtsp_parse (int idx);

int rtsp_response (int index);

void printfmemdata(char* p,int len);


#endif
