#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/uio.h>

#define		TRUE		1
#define		FALSE		0


#define USER_AGENT_MOZILLA	"mozilla"
#define USER_AGENT_OCX		"ocx"
#define USER_AGENT_IE		"ie"

#define VIDEO_TYPE_MJPEG	"mjpeg"
#define VIDEO_TYPE_ASF		"asf"
#define VIDEO_TYPE_3GP		"3gp"

#define RESOLUTION_160_120	"160x120"
#define RESOLUTION_160_128	"160x128"
#define RESOLUTION_320_240	"320x240"
#define RESOLUTION_640_480	"640x480"

#define QUALITY_VERY_LOW	"1"
#define QUALITY_LOW		"2"
#define QUALITY_NORMAL		"3"
#define QUALITY_HIGH		"4"
#define QUALITY_VERY_HIGH	"5"


typedef struct {
	char ip[32];			// ip address for the log file
	unsigned short port;		// port for the log file
	char name[32];			// user name for the log file
	char agent[8];			// user agent for the MJPEG server push - "mozilla", "ocx" and "ie"
	char video_type[8];	// video type - "mjpeg", "mpeg" and "3gp"
	char resolution[8];	// resolution - "160x128", "320x240" and "640x480"
	char quality[16];		// quality - very low to very high - "1" to "5"
	int framerate;		// 1-30
	char x_session_cookie[32];	// for rtp/rtsp over http
	char post_data[1024];	// post data queue
	int post_data_len;	// post data length
} client_message;

int pass_fd(int s);
int send_fd(int s, int fd, void *addr, socklen_t alen8);

int recv_fd_and_message (int s, client_message *pmsg);

//int pass_fd_and_message(int fd, char *usr_msg);
int pass_fd_and_message(int fd, client_message *p_msg);

#endif
