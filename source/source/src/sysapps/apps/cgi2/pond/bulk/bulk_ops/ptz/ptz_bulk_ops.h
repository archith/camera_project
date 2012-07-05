/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _PTZ_BILK_OPS_H_
#define  _PTZ_BILK_OPS_H_
#include <ptz_config.h>
#define PTZ_BULK_NAME		"PTZ"

struct PTZ_DS
{
	int  PtzMode;	
	PanTiltConf pt;
	char addpn[SC_PT_PRESET_NAME_LEN+1];
	char delpn[SC_PT_PRESET_NAME_LEN+1];
	int  PtzPanSpeed;
	int  PtzTiltSpeed;
};

int PTZ_BULK_ReadDS(void* ds);
int PTZ_BULK_CheckDS(void* ds, void* ds_org);
int PTZ_BULK_WriteDS(void* ds, void* ds_org);
int PTZ_BULK_RunDS(void* ds, void* ds_org);

#endif

