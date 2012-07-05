#define MJPEG_PAYLOAD_MAX_LENGTH	2000

#define MAX_IMG_BUF	5	//max. number of image buffer in one data packet
#define HD_BUF_NO0	13

#define MAX_LAN_BUSY 30000000
#define MAX_DRV_BUSY 1000

#define MJPEG_MAXCONNECTIONS 10


#define SINGLE_JPEG_MODE		0
#define SERVER_PUSH_JPEG_MODE	1
#define SERCOMM_MJPEG_MODE		2


#define SERCOMM_MJPEG_MAGIC_STRING		"MJPG"


/** different http header **/

#define SINGLE_JPEG_HTTP_HEADER			"HTTP/1.0 200 OK\r\n" \
										"Content-Type: image/jpeg\r\n" \
										"Content-Length: %d\r\n" \
										"Connection: close\r\n\r\n"

#define MJPEG_SERVER_PUSH_HTTP_HEADER	"HTTP/1.0 200 OK\r\n" \
										"Content-Type: multipart/x-mixed-replace;boundary=ThisRandomString\r\n"\
										"x-frame-rate: %d\r\n"\
										"x-resolution: %dx%d\r\n"

#define MJPEG_SERVER_PUSH_RANDOM_STRING	"\r\n--ThisRandomString\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\nTimestamp: %ld\r\n\r\n"
#define MJPEG_SERVER_PUSH_ENDSTRING		"\r\n--ThisRandomString--\r\n"


#define SERCOMM_MJPEG_HTTP_HEADER		"HTTP/1.0 200 OK\r\n" \
										"Connection: close\r\n\r\n"


#define COMPUTE_HEADER_SEND_STATUS(w,l,s)	if((l - s) > 0) { \
												if(w < l - s) { \
													s += w; \
													w = 0; \
												} \
												else { \
													w -= (l -s); \
													s = l; \
												} \
											}


#define COMPUTE_FRAME_SEND_STATUS(w,pl,ps,fs)	if((pl - ps) > 0) { \
													if(w < pl - ps) { \
														ps += w; \
														fs += w; \
														w = 0; \
													} \
													else { \
														w -= (pl -ps); \
														fs += (pl -ps); \
														ps = pl; \
													} \
												}




typedef struct {
    char			chktag[4];		// header marker - MJPG
    unsigned long	imgsize;		// total image size
	unsigned short	resW;			// width
	unsigned short	resH;			// height
	unsigned long	sentbytes;		// frame size
	unsigned short	sentlen;		// sent bytes of this frame
	unsigned long	timeoffset;		// timestamp
	char			frametype[1];	// frame type - 1: video 2: audio
	unsigned short	bitrate;		// kilobytes
	char			version[1];		// header version
	char			time[20];		// string of the date format
	char			reserved[2];	// reserved for further usage
} __attribute__ ((packed)) MJPEGHDR;


typedef struct {
	MJPEGHDR imghdr;	// image header (of this connection)
	MJPEGHDR audhdr;	// audio header (of this connection)

	int jpeg_type;		// single jpeg, server push and sercomm motion jpeg

	char http_header_msg[256];
	int http_header_length;
	int http_header_sent_length;

	int frame_no;
	int frame_length;
	char * frame_pointer;


	#ifdef _AUDIO_
	int a_frame_no;
	int a_frame_length;
	int a_timestamp;
	char * a_frame_pointer;

	int a_frame_header_length;

	int a_frame_sent_length;
	int a_frame_header_sent_length;
	#endif


	char byJPEGHeader[640];
	int jpeg_header_length;
	int jpeg_header_sent_length;

	char payload_header[128];

	int payload_header_length;
	int payload_length;

	int payload_header_sent_length;
	int payload_sent_length;
	int frame_sent_length;



	// 20061002 padding
	MJPEGHDR padding_header;	
	int padding_header_length;
	int padding_header_sent_length;
	
	char padding_data[128];
	int padding_length;
	int padding_sent_length;



	int base_timestamp;

	int timestamp;
	int previous_timestamp;

	unsigned int frame_count;
	unsigned int frame_old_time;
	unsigned int frame_now_time;

	char *temp_data;

} mjpeg_conn_data;

int get_padding_data_forma(int connidx);

