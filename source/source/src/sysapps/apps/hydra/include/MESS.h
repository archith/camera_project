/***************************************************************************
 *            MESS.h
 *
 *  Fri May 19 11:15:28 2006
 *  Copyright  2006  Sand Tu
 ****************************************************************************/

#ifndef _MESS_H_
#define _MESS_H_

#include <stdint.h>
#include <sys/uio.h>
#include <video/plmedia.h>
#include "MMM.h"
#include "JPEGHeader.h"

#include "rtp.h"
#include "rtsp.h"
#include "rtsp_bulk_ops.h"

#include "system_bulk_ops.h"

#include "image_bulk_ops.h"
#include "audio_bulk_ops.h"

#include "common.h"
#include "memsize.h"


#include "jpeg.h"

#ifdef _AUDIO_
#include "audio.h"
#endif

#include "timer.h"

/*
 *   Value Define
 */

/* raw image buffer count */
#define RAW_IMAGE_INFO_BUF_CNT	3

/* the maximum encoders the server support */
#define MAX_MP4_STREAM	2
#define MAX_JPG_STREAM	2
#define MAX_G726_STREAM	2

/* the maximum frame a encoder will store */
#define MAX_MP4_FRAMES	(MP4_SECOND*30)		// 30 seconds * 30 fps

//#define MAX_CAPJ_FRAMES	(JPG_SECOND_CAP*10)		// 15 seconds * 10 fps
//#define MAX_JPG_FRAMES	((JPG_SECOND_VIEW*(30-10))+ MAX_CAPJ_FRAMES)

#define MAX_VJ_FRAMES	28
#define MAX_CAPJ_FRAMES	100
#define MAX_JPG_FRAMES	(MAX_VJ_FRAMES+MAX_CAPJ_FRAMES)


#define MAX_MOBILE_FRAMES	(MOBILE_SECOND*15)	// 4 seconds * 15 fps 


/* the maximum network connection support */
#define MAX_USER_COUNTER	10
#define MAX_SESSION_COUNTER	(MAX_USER_COUNTER*2)
#define MAX_USER_ACCOUNT	32

#ifdef _EVENT_THREAD_
	#define EVENT_USER    (1<<31)
#endif


#define ALL_VTIRESOLUTION       30000
#define ALL_VTI                 1001




/*
 *   Variables declaration 
 */

#ifdef _OSD_

#define OSD_MASK_COUNT	4

struct osdtimefmt{
	int enable;
	int format;	// from 0~2, YYYY/MM/DD MM/DD/YYYY DD/MM/YYYY
	int fgcolor;
	int bgcolor;
};

struct osdmesgfmt{
	int enable;
	int fgcolor;
	int bgcolor;
	char message[IMG_MSG_LEN+1];
};

struct osdlogofmt{
	int enable;
	int x;
	int y;
	int width;
	int height;
	int logo_transparent;
	int trans_r;
	int trans_g;
	int trans_b;
};

struct maskfmt
{
	int enable[OSD_MASK_COUNT];	// enabled window
	int dimx[OSD_MASK_COUNT];	// width 
	int dimy[OSD_MASK_COUNT];	// height
	int posx[OSD_MASK_COUNT];	// position
	int posy[OSD_MASK_COUNT];	// position
	int color[OSD_MASK_COUNT];
};


typedef struct 
{
	struct osdtimefmt	time;
	struct osdmesgfmt	mesg;
	struct osdlogofmt	logo;
	int logo_option;	// Manufacturer or User-define
	int logo_fd;
	unsigned int logo_start;
	unsigned int logo_size;
	struct maskfmt		mask;
}OSD_fmt;
#endif

/* The structure of raw image */
typedef struct raw{
	// file descriptor of /dev/pl_grab
	int imagefd;

	// these are helper for calculating mapped memory address
	unsigned int mem_start;
	unsigned int mem_size;

	unsigned char *current_frame_virtual_mem_start;

	// format setting for grabber image
	GRABFORMAT	grabfmt;

	/* variables for statistic usage */
	unsigned long grab_count;
	time_t	grab_img_ts;

	IMAGEBUFFERINFO	image_buf_info[RAW_IMAGE_INFO_BUF_CNT];
	unsigned char *	virtual_mem_start[RAW_IMAGE_INFO_BUF_CNT];

#ifdef _OSD_
	OSD_fmt osd;
#endif
	
} RAW_image;

/* union of three types of connection */
typedef union{
//	asf_conn_data asf;
	mjpeg_conn_data mjpeg;
	rtp_conn_data rtp;
}asf_jpg_rtp;


#define	NET_STATE_EMPTY			0
#define NET_STATE_CONNECTED	1
#define NET_STATE_SENDING		2
#define NET_STATE_WAITING		3
#define NET_STATE_DISCONNECT	4


#define ENCODER_MP4	0x1
#define	ENCODER_JPG	0x2
#ifdef _AUDIO_
#define ENCODER_G726	0x4
#endif


/* the structure of a single sesssion */
typedef struct
{
	unsigned char state;
	int conn_fd;			// file descriptor of connection.
	int account;			// A token as an identification.
	
	/* identify the encoder this connection uses */
	unsigned char enctype;	// ENCODER_MP4, ENCODER_JPG, ENCODER_G726
	unsigned char encmp4idx;
	unsigned char encjpgidx;
	unsigned char encaudidx;
	
	int drv_busy;	// counter (unable to get image or audio)
	int lan_busy;	// counter (unable to send data)


	int last_video_type;
	unsigned long last_video_frameno;
	unsigned long last_video_timestamp;

	unsigned long time_enter_write_select;	// connection established time.

	unsigned int skip_video_frame_count;
	unsigned int error_frame_count;

	int			 drop_one_gov_flag;

	frameinfo	*fake_frameinfo;

	unsigned int bytes_sent; 
	unsigned int packets_sent;
	unsigned int frames_sent;
	
	unsigned long time_start;	// connection established time.
	
	/* frameinfo using or used */	
	frameinfo	*finfo;
	frameinfo	*prev_finfo;

	#ifdef _AUDIO_
	frameinfo	*a_finfo;
	unsigned long last_audio_frameno;
	unsigned long last_audio_timestamp;

	#endif

	struct iovec iov[13];

	// Need to make all these data structures to have the 
	// same size so that here will be a "either-or" situation
	// instead of using them all, to save some spaces here.
	// format will look like Me.conn_data[i].stream.rtp.RTP_hdr
	asf_jpg_rtp stream;

	unsigned int reversed[2];

	// client information will be shared by the log file, video access schedule, JPEG server push and assign sending task
	client_message message;
} conn_data;

/* encoded frame */

/*
 *			 |<--------cinfo------------------------->|
 *	+----------------+----------------------------------------+
 *	| viewing  queue |  capture queue			  |
 *	+----------------+----------------------------------------+
 *      |                |     ^^^^^^^
 *      |<---------------+-----+++++++   <--vinfo
 *
 *	To implement precapture feature for JPEG encoder, I divide a single queue 
 *	into two separated abstracted queue, view queue and capture queue.
 *      View queue is the queue to store the current viewing image, except the images
 *      choose to be saved in the capture queue, this queue can be managed via vinfo.
 *	Capture queue is the queue to put captured image, managed by cinfo.
 *	View queue duration is limited in 3 seconds, and capture queue is 15 seconds. (4M:12M)
 */

typedef struct encjpg{
	// file descriptor of /dev/pl_enc, using an array so that 
	// multiple streaming is possible.
	int imagefd;
	
	// these are helper for calculating mapped memory address
	unsigned char *mem_start;
	unsigned char *mem_end;
	unsigned int mem_size;
	unsigned char *mem_next;	// pointer of next encoded frame

	// helper for view queue;
	unsigned char *view_start;
	unsigned char *view_end;
	unsigned int view_size;
	unsigned char *view_next;	// pointer of next encoded frame

	// helper for capture queue
	unsigned char *cap_start;
	unsigned char *cap_end;
	unsigned int cap_size;
	unsigned char *cap_next;	// pointer of next encoded frame

	
	/* keep record of physical memory addrss and size of requested 
	   buffer */
	unsigned long phy_mem_start;
	unsigned int phy_mem_size;

	unsigned int buffer_time;
	unsigned int capture_interval;
	
	// encoded frame count of this stream
	unsigned long encode_count;
	
	// encode format setting
	ENCODEFORMAT jpgencfmt;
	int current_fps;
	int hi_rate;
	int auto_frate;

	int QLevel;
	QTABLE qTable;

	// user bitmask
	unsigned int current_usr_mask;
	unsigned long slow_connection;

	// frameinfo
	frameinfo vinfo[MAX_VJ_FRAMES];
	frameinfo cinfo[MAX_CAPJ_FRAMES];
	frameinfo *current_info; // pointer of frameinfo that encoder uses currently
	frameinfo *cap_info; // pointer of frameinfo that encoder just fulfilled
	//char hdr[MAX_JPG_FRAMES][600];	// obsolete because of the memory (600x128 bytes)

	frameinfo *current_view_info;
	frameinfo *current_cap_info;

	frameinfo *newest_frame;
}ENCODED_jpg;


typedef struct encmp4{
	// add this main switch so that we can do initialization 
	// in the very beginning.
	int enable;
	// file descriptor of /dev/pl_enc,
	int imagefd;
	
	// these are helper for calculating mapped memory address
	//|------------------------------------------------|
	//^<------------------   mem_size ---------------->^
        //+------------------------------------------------+
        //mem_start    				      mem_end		
	unsigned char *mem_start;
	unsigned char *mem_end;
	unsigned int mem_size;

	unsigned int mem_free_size;
	unsigned char *mem_next;	// pointer of next encoded frame

	unsigned int buffer_time;

	// encoded frame count of this stream
	unsigned long encode_count;
	unsigned long encode_count_p;
	
	// encoded format setting 
	ENCODEFORMAT mp4encfmt;
	int current_fps;
	int nEncodeGapRemainder;                //This value to calculate encode frame Gap.

	// remember the real fps set by the user
	int real_fps;

	// always check this flag to see if we need to do a FORCE-I encoded
	int force_I;
	
	// user bitmask
	unsigned int changed_usr_mask;
	unsigned int current_usr_mask; // changes only when encode a new
	unsigned long slow_connection;

	// frameinfo
	frameinfo vinfo[MAX_MP4_FRAMES];
	frameinfo *current_info;

	frameinfo *newest_frame;
	frameinfo *newest_iframe;
	frameinfo *newest_pframe;
}ENCODED_mp4;

#ifdef _AUDIO_
typedef struct
{
	// file descriptor of /dev/pl_grab
	int audiofd;
	
	int sample_size;
	int sample_rate;
	int channel;
	
	char buffer[1024*10];
	int buf_cur_len;
	int buf_max_len;

}RAW_audio;

typedef struct {
	frameinfo finfo;
	char data[1200];
} AUDIO_frame_data;

typedef struct
{
	// encoded frame count of this stream
	unsigned long encode_count;

	// user bitmask
	unsigned int current_usr_mask;

	int lock_state;


	AUDIO_frame_data frames[AUDIO_SLOT_NUM];

	frameinfo *head_info;
	frameinfo *current_info;
	frameinfo *newest_frame;
} ENCODED_g726;
#endif

/* The structure of encoders */
typedef struct {
	// using an array so that multiple streaming is possible.
	ENCODED_mp4 mp4[MAX_MP4_STREAM];
	ENCODED_jpg jpg[MAX_JPG_STREAM];
#ifdef _SNAPSHOT_
#define SNAPNONE	0x00
#define SNAPINIT	0x01
#define SNAPDONE	0x02
#define SNAPSHOT	0x03
#define GRAYBUF_SIZE    491520 //Y:640*512 U:320*250 V:320*256
	unsigned char snapshotcmd:2,
		      snapshotidx:2,
		      snapshotcnt:4;
#endif
#ifdef _SW_ENCODE_JPEG_
        int snap_res;        /* 5:128x96 , 6:64x48 */
        int snap_qual;    /* Normal:1024 */
        int snap_color;      /* 0:Gray(YUV400) 1:Color(YUV422) */
        unsigned char snap_yuvaddr[GRAYBUF_SIZE];
#endif
#ifdef _AUDIO_
	ENCODED_g726 g726[MAX_G726_STREAM];
#endif
} ENCODER;

#ifdef _MOTION_
#define DC_FLAG         1
#define MV_FLAG         2

#define W1_DETECTED     0x01
#define W2_DETECTED     0x02
#define W3_DETECTED     0x03
#define W4_DETECTED     0x04
#define FRATE_PULL_H    0x10
#define FRATE_PULL_L    0x20
#define FRATE_KEEP_CUR  0x30
typedef struct
{
	// file descriptor of /dev/pl_md
	int motionfd;
	
	unsigned char *MD_start;
	unsigned char *pMD_start;
	unsigned int buffer_length;

	// motion detection settings	
	int motion_enable;
	int indicator_enable;	//while disable, user turn on and need indicator value
	int encidx;		// The encoder we are watching	
	int time_interval;
	int position_x;
	int position_y;
	char en_area;		// 4 bits for each window, 1 bit for all;
	
	/* any of this flag is set, ignore result of chk_MD_value */
	unsigned char is_initiate:1,
		      overlay_tick:1,
		      motor_move:1,
		  	motor_position:1,
			resv:4;

	char paddings[MOT_PADDING_LEN];
	// result will be saved here
 	time_t trg_tm;  //jeffrey add

	int attach_ccdc_shm;	// 1: success, 0: fail
}MD_value;
#endif





/* MESS status, this structure will be store in a shared memory so that the 
   other program can retrieve these data easily. */
typedef struct
{
	/* system */ 
	char szhost[DEV_LEN+1];	//Use in multcast sdp of session name.

	unsigned long Me_offset;
	/* global variables */
	int total_user_counter;
	int asf_user_counter;
	int mjpeg_user_counter;
	int rtp_user_counter;	
	int maxfd;
	time_t timeStart;

	// network
	/* sock from webserver */
	int unixfd;
	/* sock from internet */
	int rtspfd;
	rtsp_conf rtspconf;
	rtsp_req_t rtsp_conn[MAX_USER_COUNTER];	
	char mobile_access_name[17];
	
	/* stream connections */
	conn_data conn[MAX_SESSION_COUNTER];

	// raw image 
	RAW_image raw;

#ifdef _AUDIO_
	int audio_enable;
	int audio_encoder_type;
	int audio_reset_flag;

	int audio_half_mode;
	// raw audio
	RAW_audio raw_audio;
#endif
	// encoded data
	ENCODER encoded;

#ifdef _MOTION_
	// motion detection value
	MD_value	motion;
#endif

#ifdef _EVENT_THREAD_
	unsigned short is_triggered; // bit2: mot, bit1: in2, bit0: in1
	frameinfo *video_head;
	frameinfo *video_tail;
	frameinfo *audio_head;
	frameinfo *audio_tail;
	int event_conf_chg;
#endif

	// schedule & priority

	// user account token pool
	int user_acnt_summary;
} MESS_engine;

typedef struct 
{
	unsigned int frame_count;
	unsigned long start_time;
	unsigned long interval;
	char description[128];
} FrameRateUnit ;

#define  PFrameRateUnit FrameRateUnit*

/*
 *	 Functions Declaration
 */

/* network.c */
int init_network(void);
int close_network(void);
		
int init_rtsp_socket (int port);
int rtsp_conn_closeEx(int index);
int rtsp_conn_close (int index,int connIdx);

int find_free_dataconn (void);
int find_free_rtspconn (void);

int accept_rtsp_connection(int fd);
int accept_unix_connection(int fd);

int destory_mcast_conn(void);
int create_mcast_conn(void);

void process_send_img (int cid);

int setkeepalive (int sock);

/* signal.c */
void init_signal_handler(void);

/* option.c */
int MESS_reload_config(void);
int MESS_read_config(void);


/* encode.c */
int init_raw_grabber(void);
int close_raw_grabber (void);

int init_mp4_encoder (int index);
int encode_mp4 (READYINFO *ReadyInfo, int mindex, int nCodingType);
int close_mp4_encoder (int index);

int init_jpg_encoder (int index);
int encode_jpg (READYINFO *ReadyInfo, int jindex);
int close_jpg_encoder (int index);
int grab_and_encode(void);

int set_padding_data_format(char * buf, frameinfo *finfo);
int get_video_frame(int cidx);
int free_video_frame(int cidx);
int free_all_video_frame(int cidx);
#ifdef _AUDIO_
int get_audio_frame(int cidx);
int free_audio_frame(int cidx);
int free_all_audio_frame(int cidx);
int start_to_lock_audio_frames(int acnt, unsigned int ts);
int stop_locking_audio_frames(int acnt);
int get_locked_audio_frames(int acnt);
int unlock_audio_frames(int acnt);
#endif

/* volhdr.c */
int gen_mpeg4_header(uint8_t *buffer, int codec_type,
                     int width, int height,
                     uint8_t quant_type,
                     uint8_t with_vp_header, int vti_res);
/* umgr.c */
int update_user_cnt(void);
int request_acnt (int cid);
int release_acnt (int cid);

/* tstamp.c */

int getsec_init(void);
unsigned long getmsec(void);

/* snapshot.c */
int init_snapshot_encoder (int index);
int encode_snapshot(READYINFO *ReadyInfo, int jindex);
int close_snapshot_encoder (int index);

#ifdef _MJPG_
/* jpeg.c */
void mjpeg_process_connection(int connidx);
int mjpeg_arming(int connidx);
int mjpeg_launch(int connidx);
void mjpeg_close_connection(int connidx);

/* jpg_qtbl.c */
int jpeg_get_qtbl(unsigned char lumtbl[], unsigned char chmtbl[], int quality);
#endif

/*osd.c */
int osd_draw_raw(int index);
int osd_draw_label(int nIndex, int scale);
int on_screen_init(void);
int on_screen_close(void);
void logo_restart(void);
/* md.c */
#ifdef _MOTION_
int mot_attach_shm(void);
int mot_detach_shm(void);
int chk_MD_value(MD_value *md);
int init_MD_device(void);
int close_MD_device (void);
#endif

/* log.c */
void mp4_buffer_log();
void connection_status_log();
void common_log(const char *format, ...);

/* watchdog.c */
void renewTimer();
void checkTimer();

#endif /* _MESS_H_ */
