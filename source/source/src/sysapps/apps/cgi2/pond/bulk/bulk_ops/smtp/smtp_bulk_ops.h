/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*	Use of this software is restricted to the terms and
*	conditions of SerComm's software license agreement.
*
*			www.sercomm.com
****************************************************************/
#ifndef _SMTP_BULK_OPS_H_
#define _SMTP_BULK_OPS_H_

#define		SMTP_BULK_NAME		"SMTP"

#define		SERVER_NUM		2

#define		SMTP_AddressLen		48
#define		SMTP_SubjectLen		48
#define		SMTP_UrlLen		48
#define		SMTP_ToUser		3
#define 	SMTP_SrvLen		64
#define		SMTP_UserLen		32	//define temp.
#define		SMTP_PassLen		16	//define temp.
#define		SMTP_MessageLen		512

#define		SMTP_MAX_PORT		65535
#define		SMTP_MIN_PORT		0

enum{
	SMTP_AUTH_NONE = 0,
	SMTP_AUTH_SMTP,
	SMTP_AUTH_POP
};

/* smtp error code */
#define		SMTP_EENABLE		-100
#define		SMTP_ESMTPSRV		-101
#define		SMTP_EPOPSRV		-102
#define		SMTP_EUSER		-103
#define		SMTP_EPASSWD		-104
#define		SMTP_EADDRESS		-105


struct SMTP_DS{
	char	to_addr[SMTP_ToUser][SMTP_AddressLen+1];	// recipients address
	int 	to_att_enable;
	char	from_addr[SMTP_AddressLen+1];	// from address
	char	subject[SMTP_SubjectLen+1];	// subject
	int	enable_video_url;
	char	video_url[SMTP_UrlLen+1];

	char	smtp_server[SERVER_NUM][SMTP_SrvLen+1];	// smtp server
	int	smtp_auth[SERVER_NUM];			// 0:no 1:smtp auth 2:pop auth
	char	smtp_user[SERVER_NUM][SMTP_UserLen+1]; 	// login name
	char	smtp_pw[SERVER_NUM][SMTP_PassLen+1];		// login pw
	int	smtp_port[SERVER_NUM];			// smtp server listen port
	char	smtp_mesg[SMTP_MessageLen+1];
        int     smtp_ssl[SERVER_NUM];

	int	enable[SERVER_NUM];				// smtp function enable
	char    pop_server[SERVER_NUM][SMTP_SrvLen+1];
	int	rcpt_enable;			// bit0: to_addr[0] enable/disable
						// bit1: to_addr[1]
};


int SMTP_BULK_ReadDS(void* ds);
int SMTP_BULK_CheckDS(void* ds, void* ds_org);
int SMTP_BULK_WriteDS(void* ds, void* ds_org);
int SMTP_BULK_WebMsg(int errcode, char* message, int* type);
#endif

