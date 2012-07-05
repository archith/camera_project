#ifndef _AVI_INTERFACE_H_
#define _AVI_INTERFACE_H_
#include <errno.h>
#include <sys/uio.h>
#include "avi_format.h"

/* max array size of frame entry */
#define MAX_VFRAME_NUM		((10+2)*30)
#define MAX_AFRAME_NUM		((10+2)*8)
#define	MAX_CHUNK_NUM		(MAX_VFRAME_NUM+MAX_AFRAME_NUM)
#define MAX_IOVEC_NUM		(MAX_CHUNK_NUM*10+20)


#define	ERR_LT(a,b)	\
	{if(a < b){fprintf(stderr, "<%s: %d> %d < %d\n", __FILE__, __LINE__, a, b); return -1;}}

//#define _AVI_DEBUG_
#ifdef	_AVI_DEBUG_
#define AVI_DEBUG(fmt, arg...);	\
	{fprintf(stderr, "<%s: %d> ", __FILE__, __LINE__); fprintf(stderr, fmt, ##arg);}
#else
#define AVI_DEBUG(fmt, arg...);
#endif

typedef struct iovec	AVI_VEC;

typedef	struct{
	DWORD	codec;
	FOURCC	stream_id;	// stream chunk id
	DWORD	bpp;		// bits per pixel
	DWORD	max_fps;
	DWORD	max_fsz;
	DWORD	length;		// stream length (sec)
	DWORD	total_frm;	// total number of frames
	DWORD	width;
	DWORD	height;
	AVI_VEC	vol;		// vol header of mpeg4 stream
}AVI_VIDEO_INFO;
typedef struct{
	BYTE	enable;
	WORD	codec;
	FOURCC	stream_id;	// stream chunk id
	DWORD	bps;		// bits per sample
	DWORD	max_sps;	// max samples per second
	DWORD	max_fsz;
	DWORD	length;		// stream length (sec)
	DWORD	total_frm;
	DWORD	n_channel;
}AVI_AUDIO_INFO;
#define MAX_FRAME_VECTOR	7
typedef struct{
	AVI_VEC	frm_data[MAX_FRAME_VECTOR];
	DWORD	timestamp;
	BYTE	not_keyfrm;
	DWORD	real_size;	// real size after add some other info in the chunk
	BYTE	padding_size;	// padding zero length
#if (defined _USE_NEW_INDEX_)
	AVIStdIndexEntry index;
#elif (defined _USE_OLD_INDEX_)
	AviOldIndexEntry index;
#endif
}AVI_FRAME_ENTRY;
typedef struct{
	AVI_FRAME_ENTRY vfrm[MAX_VFRAME_NUM+1];
	AVI_FRAME_ENTRY afrm[MAX_AFRAME_NUM+1];
}AVI_ENTRY;
typedef struct{
#if (defined _USE_NEW_INDEX_)
	AVIStdIndexEntry idx;
#elif (defined _USE_OLD_INDEX_)
	AviOldIndexEntry idx[MAX_CHUNK_NUM];
#endif
	size_t	cnt;
}AVI_INDEX;
typedef struct{
	DWORD	zero;		// 0
}AVI_ELEMENT;
#define VEC_HEADER	1
#define VEC_DATA	2
#define VEC_INDEX	3
typedef struct{
	AVI_VEC	v[MAX_IOVEC_NUM];
	size_t	cnt;		// iovec count
	AVI_VEC	index_v[MAX_CHUNK_NUM+1];
	size_t	index_cnt;
	size_t	header_len;	// size before movi data start
	size_t	data_len;	// size after movi data start
	size_t	index_len;	// size of all the index entries
}AVI_DATA_VEC;
typedef struct{
	AVI_VIDEO_INFO	video;
	AVI_AUDIO_INFO	audio;
	AVI_ENTRY	entry;
	AVI_INDEX	index;
	AVI_ELEMENT	element;
	AVI_DATA_VEC	data;
}AVI_INFO;


#endif	// _AVI_INTERFACE_H_
