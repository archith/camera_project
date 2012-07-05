/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2007.
*
*       Use of this software is restricted to the terms and
*       conditions of SerComm's software license agreement.
*
*                       www.sercomm.com
****************************************************************/

#ifndef  _CDP_BULK_OPS_H_
#define  _CDP_BULK_OPS_H_

#define CDP_BULK_NAME		"CDP"

#define GRAPHIC                 1
#define NUM_ALPHA               2


#define CDP_EENABLE		-100


typedef struct CDP_DS {
	int  f_enable;		 	// 0: disable, 1:enable
} CDPConfig;


int CDP_BULK_ReadDS(void* ds);
int CDP_BULK_CheckDS(void* ds, void* ds_org);
int CDP_BULK_WriteDS(void* ds, void* ds_org);
int CDP_BULK_RunDS(void* ds, void* ds_org);
int CDP_BULK_WebMsg(int errcode, char* message, int* type);
#endif
