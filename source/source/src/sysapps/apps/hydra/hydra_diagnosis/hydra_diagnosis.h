#include <stdio.h>
#include <string.h>

/* hydra_diagnosis_if.c */
int usage(int argc, char *argv[]);
int parse_command(int argc, char *argv[]);

/* hydra_diagnosis_shm.c */
int attach_shm(void);
int detach_shm(void);

/* hydra_diagnosis_mpeg1.c */
void list_video_frame_detail();
void diagnose_mpeg4_1_encoder();

/* hydra_diagnosis_mobile.c */
void list_mobile_frame_detail();
void diagnose_mpeg4_2_encoder();

/* hydra_diagnosis_audio.c */
void diagnose_audio_encoder();
void list_audio_frame_detail();

/* hydra_diagnosis_users.c */
void list_all_users_detail();

#define COMMAND_LIST_FIRST_VIDEO_FRAME_INFO	1
#define COMMAND_LIST_FIRST_VIDEO_ENCODE_INFO	2

#define COMMAND_LIST_SECOND_VIDEO_FRAME_INFO	10
#define COMMAND_LIST_SECOND_VIDEO_ENCODE_INFO	11

#define COMMAND_LIST_AUDIO_FRAME_INFO		20
#define COMMAND_LIST_AUDIO_ENCODE_INFO		21

#define COMMAND_LIST_CONNECTION_INFO		30

#define COMMAND_UNKNOWN				100

#define DIAGNOSIS_VERSION			"V0.0.2"
