/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*	Use of this software is restricted to the terms and
*	conditions of SerComm's software license agreement.
*
*			www.sercomm.com
****************************************************************/
#ifndef _EVENT_BULK_OPS_H_
#define _EVENT_BULK_OPS_H_

#define EVENT_BULK_NAME	"EVENT"

/* trigger action number */
#define TRIG_ACT_NUM	6
/* trigger action flags */
#define TRIG_OUT1_BIT	0
#define TRIG_OUT2_BIT	1
#define TRIG_EMAIL_BIT	2
#define TRIG_FTP_BIT	3
#define TRIG_IM_BIT	4
#define TRIG_SMB_BIT	5
/* file type */
#define TRIG_TYPE_MPG4	0
#define TRIG_TYPE_JPEG	1
/* mpeg4 video file format */
#define TRIG_FILE_ASF	1
#define TRIG_FILE_MP4	2
#define TRIG_FILE_3GP	3
#define TRIG_FILE_AVI	4
#define TRIG_FILE_JPG	5


typedef struct captured_mp4{
	int mp4_fmt;	// 1 - asf, 2 - mp4, 3 - 3gp, 4 - avi;
	int pre_len;	// video pre capture length (secs),
	int post_len;	// video post capture length (secs)
}CAPTURED_MP4;

typedef struct captured_jpg{
	int jpg_fmt;		// 1 - jpg, 2 - avi;
	int jpg_frmrate;	// jpg frame rate
	int pre_len;	// jpg pre capture length (secs)
	int post_len;	// jpg post capture length (secs)
}CAPTURED_JPG;

struct EVENT_DS{
	int en_trig;
	int interval;		// event triggered interval (secs)
	int in1_action;	// same as "action" declared in IMG_QUEUE_S
	int in2_action;
	int mot_action;
	int img_type;		// attached image type: 0-mpeg4, 1-jpeg
	int file_overwrite_enable; // overwrite video log file
	CAPTURED_MP4 mp4conf;
	CAPTURED_JPG jpgconf;
};



int EVENT_BULK_ReadDS(void* ds);
int EVENT_BULK_CheckDS(void* ds, void* ds_org);
int EVENT_BULK_WriteDS(void* ds, void* ds_org);
int EVENT_BULK_RunDS(void* ds, void* ds_org);

#endif

