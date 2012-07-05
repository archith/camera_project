/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*       Use of this software is restricted to the terms and
*       conditions of SerComm's software license agreement.
*
*                       www.sercomm.com
****************************************************************/

#ifndef  _FTP_BULK_OPS_H_
#define  _FTP_BULK_OPS_H_

#define FTP_BULK_NAME		"FTP"

#define SRV_LEN	       	        64
#define USER_LEN	        32
#define PASSWD_LEN              16
#define PATH_LEN                64  
#define IMG_FILE_LEN	       	64

#define MAX_PORT_NUM		65535
#define MIN_PORT_NUM		0

#define GRAPHIC			1
#define NUM_ALPHA		2

/* ftp error code */
#define	FTP_EENABLE		-100
#define	FTP_ESERVER		-101
#define	FTP_EUSER		-102
#define	FTP_EPASSWD		-103

typedef struct FTPConfig {
	int  f_enable;		 	// 0: disable, 1:enable
	char f_sname[SRV_LEN+1];
	char f_user[USER_LEN+1];
	char f_passwd[PASSWD_LEN+1];
        char f_path[PATH_LEN+1];
        char f_fname[IMG_FILE_LEN+1];
	int  f_port;
	int  f_passive;  		// 0: passive off , 1: passive on
} FTPConfig;

struct FTP_DS{
	FTPConfig f_conf[2];
};


int FTP_BULK_ReadDS(void* ds);
int FTP_BULK_CheckDS(void* ds, void* ds_org);
int FTP_BULK_WriteDS(void* ds, void* ds_org);
int FTP_BULK_WebMsg(int errcode, char* message, int* type);
#endif
