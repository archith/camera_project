/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _QOS_BULK_OPS_H_
#define  _QOS_BULK_OPS_H_

#define QOS_BULK_NAME "QOS"

struct QOS_DS
{
	int qos_dscp_value;	//0 - 63
	int qos_enable;		//1:enable 0:disable
	int qos_av_switch;	//0:audio 1:video
	int cos_enable;		// 1: enable 0: disable
	int cos_priority;	// 0 ~ 7
	int cos_vlan_id;	// 1 ~ 4094
};

#define QOS_THIS_IS_AUDIO_STREAM 	0
#define QOS_THIS_IS_VIDEO_STREAM 	1
#define QOS_THIS_IS_AV_STREAM 		2

int QOS_BULK_ReadDS(void* ds);
int QOS_BULK_CheckDS(void* ds, void* ds_org);
int QOS_BULK_WriteDS(void* ds, void* ds_org);
int QOS_BULK_RunDS(void* ds, void* ds_org);
#endif
