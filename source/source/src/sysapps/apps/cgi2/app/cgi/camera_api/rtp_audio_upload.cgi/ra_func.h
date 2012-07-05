#ifndef _RA_FUNC_
#define _RA_FUNC_


#define MAX_CMD_LENGTH		64
#define CMD_TABLE_LENGTH	4

#define RA_CREATE_CMD	"CreateAudioRTP"
#define RA_START_CMD	"StartAudioRTP"
#define RA_STOP_CMD		"StopAudioRTP"
#define RA_DESTROY_CMD	"DestroyAudioRTP"

#define RA_CREATE_ID	0
#define RA_START_ID		1
#define RA_STOP_ID		2
#define RA_DESTROY_ID	3


typedef int (*func)(char *para);

int create_rtp_audio(char *para);
int start_rtp_audio(char *para);
int stop_rtp_audio(char *para);
int destroy_rtp_audio(char *para);



typedef struct 
{
	char cmd[MAX_CMD_LENGTH];
	int id;
	func exec;
} cmd_type;

cmd_type cmd_table[CMD_TABLE_LENGTH] = {
	{RA_CREATE_CMD,	RA_CREATE_ID, create_rtp_audio},
	{RA_START_CMD,	RA_START_ID, start_rtp_audio},
	{RA_STOP_CMD,	RA_STOP_ID, stop_rtp_audio},
	{RA_DESTROY_CMD, RA_DESTROY_ID, destroy_rtp_audio}
};

#endif
