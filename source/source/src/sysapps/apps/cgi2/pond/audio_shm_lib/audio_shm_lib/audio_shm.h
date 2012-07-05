#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define AUDIO_UPLOAD_SHMKEY				((key_t) 2829)
#define LINKSYS_ONE_AUDIO_CGI_SHMKEY	((key_t) 2830)

#define PERMS           0666
#define PAGE_NUM        5

typedef struct
{
	// rtp/http common item
	int uploading;			// for simplex and duplex
	int connection_fd;

	// rtp audio upload item
	int rtp_status;			// rtp status - empty, create, destory
	int accept;				// accepted codec - g.711a, g.711u, g.729
	int max_packet;			// packet number per second - 100, ..., 1000
	char local_ip[24];		// exported ip for the client to upload audio
	int local_port;			// port to listen the uploaded audio

	// rtp audio download item
	int remote_standby;		// 0: empty, 1: sending
	char remote_ip[24];		// send the rtp audio to remote ip
	int remote_port;		// send the rtp audio to remote port
} audio_upload_shared_memory;


#if 0
typedef struct 
{
	char version:2;
	char padding:1;
	char extension:1;
	char csrc_count:4;

	char marker:1;
	char payload_type:7;

	unsigned short sequence_number;
	unsigned int timestamp;
	char ssrc_identifier[4];
}  __attribute__((packed)) RTP_header;
#else
#if 0
typedef struct 
{
	char csrc_count:4;
	char extension:1;
	char padding:1;
	char version:2;

	char payload_type:7;
	char marker:1;

	unsigned short sequence_number;
	unsigned int timestamp;
	char ssrc_identifier[4];
}  __attribute__((packed)) RTP_header;
#endif

typedef struct 
{
	unsigned short csrc_count:4;
	unsigned short extension:1;
	unsigned short padding:1;
	unsigned short version:2;
	unsigned short payload_type:7;
	unsigned short marker:1;

	unsigned short sequence_number;
	unsigned int timestamp;
	char ssrc_identifier[4];
}  __attribute__((packed)) RTP_header;

#endif

int * get_audio_upload_shared_memory();
int * get_linksys_one_audio_cgi_shared_memory();
