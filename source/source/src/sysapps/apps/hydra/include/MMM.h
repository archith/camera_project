/***************************************************************************
 *            MMM.h
 *	Multi-Memory Management component
 *
 *  Mon May 22 18:40:08 2006
 *  Copyright  2006  User
 *  Email
 ****************************************************************************/

#ifndef _MMM_H
#define _MMM_H

#define MEM_UNIT_SHIFT	10	// 1<< 10 == 1024

#define ENCODED_JPG_BUF_CNT 4	// an internal value

#define MOT_PADDING_LEN		13
#define IN_PADDING_LEN		2


// all these values are based on assumption 
#define MAX_JPG_640_SIZE	(256*1024)	//256KB
#define MAX_JPG_320_SIZE	(64*1024)	//64KB
#define MAX_JPG_160_SIZE	(16*1024)	//16KB



typedef struct fi{
 	unsigned int *start_addr;
 	unsigned int offset;
	unsigned int length;
	unsigned short vop:1,
		       is_cap:1,	//is captured frame or not.
	  	      unit:14;
	unsigned long frameno;
	
	unsigned long timestamp;
	
	// buffer reference
	unsigned int ref_usr_cnt;

	// motion detect value
	char thre_ind_en[MOT_PADDING_LEN];	// Threshold, Indicator, Enable
	char input_padding[IN_PADDING_LEN];	// input status padding

	// MPEG4/JPEG size scale
	int scale;

	// JPEG header
	unsigned char jpg_qlv;

	// linked-list search operations
	struct fi *prev;		// pointer of previous frameinfo
	struct fi *next;		// pointer of next frameinfo
}frameinfo;


#define METHOD_REWIND	0x01
#define	METHOD_FORWARD	0x02

#define	METHOD_NOREF	0x04
#define	METHOD_TSTAMP	0x08
#define METHOD_SEQUENCE	0x10
#define	METHOD_USER		0x20

#define	METHOD_MIN_TSTAMP	0x40


struct search_paras
{
	int method;
	int type;	
	unsigned long ts;
	unsigned long seq;
	unsigned int user;
	frameinfo *current;
};

typedef struct search_paras search_parameter;

int calculate_mp4_buffer_size(int idx);
int calculate_jpg_buffer_size(int idx);

void assign_physical_addr_to_jpeg_frameinfo(int idx);
frameinfo *search_frame( search_parameter *p );

#endif /* _MMM_H */
