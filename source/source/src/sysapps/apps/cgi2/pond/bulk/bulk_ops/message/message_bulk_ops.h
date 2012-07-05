/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _MESSAGE_BULK_OPS_H_
#define  _MESSAGE_BULK_OPS_H_

#include <profile.h>
#define MESSAGE_BULK_NAME	"MESSAGE"

struct MESSAGE_DS
{
	int type;
	char message[PRO_VALUE_MAX_LEN+1];
	char next_file[64+1];
};
int MESSAGE_BULK_ReadDS(void* ds);
int MESSAGE_BULK_RunDS(void* ds, void* ds_org);

#endif
