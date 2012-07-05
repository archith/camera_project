/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _DNSWITCH_BULK_OPS_H_
#define  _DNSWITCH_BULK_OPS_H_

#define DNSWITCH_BULK_NAME "DNSWITCH"

struct DNSWITCH_DS
{
	int dn_filter;
	int dn_sch;
	int dn_sch_hr;
	int dn_sch_min;
	int dn_hrend;
	int dn_minend;
};

int DNSWITCH_BULK_ReadDS(void* ds);
int DNSWITCH_BULK_CheckDS(void* ds, void* ds_org);
int DNSWITCH_BULK_WriteDS(void* ds, void* ds_org);
int DNSWITCH_BULK_RunDS(void* ds, void* ds_org);
#endif
