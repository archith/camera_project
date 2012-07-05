/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*	Use of this software is restricted to the terms and
*	conditions of SerComm's software license agreement.
*
*			www.sercomm.com
****************************************************************/

#ifndef _USER_BULK_OPS_H_
#define _USER_BULK_OPS_H_

#define USER_BULK_NAME	"USER"

#define USER_USER_START_NUM	1
#define USER_USER_END_NUM	20

#define USER_MAX_USER_NUM	20



#define	USER_ID_LEN	32
#define	USER_PW_LEN	64


typedef struct{
	char id[USER_ID_LEN+1];
	char pw[USER_PW_LEN+1];
}ACCOUNT_INFO;

struct USER_DS{
	ACCOUNT_INFO	adminfo;
	ACCOUNT_INFO	vinfo;
	ACCOUNT_INFO	uinfo[USER_MAX_USER_NUM];
	int		adm_timeout;	// 0 - disable, other timeout(minute)
	int		adm_ctrl;	// bit 0: user1, bit 19: user20
	int		io_ctrl;
	int		pt_ctrl;
	int		monitor_ctrl;	// io_ctrl & pt_ctrl
};

int USER_BULK_ReadDS(void* ds);
int USER_BULK_CheckDS(void* ds, void* ds_org);
int USER_BULK_WriteDS(void* ds, void* ds_org);
int USER_BULK_RunDS(void* ds, void* ds_org);
int USER_BULK_WebMsg(int errcode, char* message, int* type);
#endif
