/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _MISC_BULK_OPS_H_
#define  _MISC_BULK_OPS_H_

#define MISC_BULK_NAME "MISC"
struct MISC_DS
{
	char todo[128];
};
int MISC_BULK_ReadDS(void* ds);
int MISC_BULK_CheckDS(void* ds, void* ds_org);
int MISC_BULK_WriteDS(void* ds, void* ds_org);
int MISC_BULK_RunDS(void* ds, void* ds_org);
int MISC_BULK_WebMsg(int errcode, char* message, int* type);
#endif

