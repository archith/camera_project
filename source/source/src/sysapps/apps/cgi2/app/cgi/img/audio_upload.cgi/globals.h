#include <stdio.h>
#include <pthread.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>

#include "audio_common.h"

/* Message to User */
#define MSG_OK				"HTTP/1.1 200 OK\r\n\r\n"
#define MSG_ERROR_CONFLICT	"HTTP/1.1 400 Bad Request\r\n\r\n<HTML><HEAD><TITLE>Bad Request</TITLE></HEAD><BODY>There is another user uploading the audio.</BODY></HTML>"
#define MSG_ERROR_DISABLE	"HTTP/1.1 400 Bad Request\r\n\r\n<HTML><HEAD><TITLE>Bad Request</TITLE></HEAD><BODY>The audio is disabled.</BODY></HTML>"


/* Buffer Configuration */
//#define BUFFER_IN_LENGTH	1000
//#define BUFFER_OUT_LENGTH	8000

//#define BUFFER_IN_LENGTH	32
//#define BUFFER_OUT_LENGTH	256

#define BUFFER_IN_LENGTH      256
#define BUFFER_OUT_LENGTH     2048

#define BUFFER_NUMBER		8

/* Buffer Status */
#define BUFFER_STATUS_EMPTY		0
#define BUFFER_STATUS_FILLING	1
#define BUFFER_STATUS_FILLED	2
#define BUFFER_STATUS_PLAYING	3

#define FILE_NAME_G711_ALAW		"g711a.cgi"
#define FILE_NAME_G711_ULAW		"g711u.cgi"
#define FILE_NAME_G726			"g726.cgi"

#define DECODE_TYPE_G711_ALAW	0
#define DECODE_TYPE_G711_ULAW	1
#define DECODE_TYPE_G726		2



/* Lan Timeout */
#define MAX_LAN_BUSY			10000


typedef struct {
	int status;

	int in_length;
	int out_length;

	char in_buffer[BUFFER_IN_LENGTH];
	char out_buffer[BUFFER_OUT_LENGTH];
} BUFFER_INFO;


