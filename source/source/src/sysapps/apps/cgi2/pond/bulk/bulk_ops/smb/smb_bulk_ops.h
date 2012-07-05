/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _SMB_BULK_OPS_H_
#define  _SMB_BULK_OPS_H_


#define SMB_BULK_NAME 			"SMB"
#define SAMBA_LOGFILE_GROUP		"SAMBA"
#define SMB_SERVER_LEN			64 
#define SMB_PATH_LEN			128  
#define SMB_USER_LEN			64
#define SMB_PASSWD_LEN			64
#define SMB_GROUP_LEN			64

struct SMB_DS
{
	int  enable;		 	// 0: disable, 1:enable
	char server[SMB_SERVER_LEN+1];
	char path[SMB_PATH_LEN+1];	
	char user[SMB_USER_LEN+1];
	char passwd[SMB_PASSWD_LEN+1];
	//char group[SMB_GROUP_LEN+1];
	int  rec_enable;			// 0: disable, 1:enable
	int  rec_filesize;
	int  rec_mode;
	char rec_server[SMB_SERVER_LEN+1];
	char rec_path[SMB_PATH_LEN+1];	
	char rec_user[SMB_USER_LEN+1];
	char rec_passwd[SMB_PASSWD_LEN+1];
};
int StopSmbcRec(void);
int StartSmbcRec(void);
int SMB_BULK_ReadDS(void* ds);
int SMB_BULK_CheckDS(void* ds, void* ds_org);
int SMB_BULK_WriteDS(void* ds, void* ds_org);
int SMB_BULK_RunDS(void* ds, void* ds_org);
#endif

