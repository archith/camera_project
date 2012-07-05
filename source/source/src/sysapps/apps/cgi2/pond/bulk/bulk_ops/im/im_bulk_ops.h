/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _IM_BULK_OPS_H_
#define  _IM_BULK_OPS_H_

#define IM_DS_SERVER_LEN 		64
#define IM_DS_ACCOUNT_LEN 		32
#define IM_DS_PASSWORD_LEN 		64
#define IM_DS_SENDTO_LEN 		128
#define IM_DS_MESSAGE_LEN 		256
#define IM_BULK_NAME "IM"

struct IM_DS
{
	int enable;
	char server[IM_DS_SERVER_LEN+1];
	char account[IM_DS_ACCOUNT_LEN+1];
	char password[IM_DS_PASSWORD_LEN+1];
	char sendto[IM_DS_SENDTO_LEN+1];
	char message[IM_DS_MESSAGE_LEN+1];
};
int IM_Stop(void);
int IM_Start(void);
int IM_BULK_ReadDS(void* ds);
int IM_BULK_CheckDS(void* ds, void* ds_org);
int IM_BULK_WriteDS(void* ds, void* ds_org);
int IM_BULK_RunDS(void* ds, void* ds_org);
int IM_BULK_WebMsg(int errcode, char* message, int* type);
#endif
