/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _RTSP_BULK_OPS_H_
#define  _RTSP_BULK_OPS_H_

#define RTSP_BULK_NAME   		SEC_RTSP_RTP
#define RTSP_OK                         0
#define RTSP_READ_ERR			-1
#define RTSP_SAVE_ERR			-2
#define RTSP_RESTART_ERR		-3

#define DEFAULT_RTSP_PORT	554
#define DEFAULT_RTP_PORT	1024
#define DEFAULT_RTP_PKT_LEN     1400
#define DEFAULT_MCAST_ENABLE	0
#define DEFAULT_MCAST_VIDEO_ADDR	"224.2.0.1"
#define DEFAULT_MCAST_VIDEO_PORT	2240
#define DEFAULT_MCAST_AUDIO_ADDR	"224.2.0.1"
#define DEFAULT_MCAST_AUDIO_PORT	2242
#define DEFAULT_MCAST_TTL	16

#define DEFAULT_MCAST_GROUP_LEN	16

typedef struct rtsp_conf{
	int rtsp_port;
	int rtp_port;
        int rtp_pkt_len;
	int mcast_enable;
	char mcast_video_addr[16];
	int mcast_video_port;
	char mcast_audio_addr[16];
	int mcast_audio_port;
	int mcast_ttl;
	char mcast_group[DEFAULT_MCAST_GROUP_LEN+1];
} rtsp_conf;

/*
 * purpose: Read rtsp info from configuration file 
 * return: RTSP_OK	- succeed	
 *   	    others 	- fail
 */
int RTSP_ReadConf(void *ds);
int RTSP_BULK_CheckDS(void* ds, void* ds_org);
int RTSP_WriteConf(void *ds, void *ds_org);
int RTSP_RunConf(void *conf, void *conf_org);
int RTSP_BULK_WebMsg(int errcode, char* message, int* type);
#endif
