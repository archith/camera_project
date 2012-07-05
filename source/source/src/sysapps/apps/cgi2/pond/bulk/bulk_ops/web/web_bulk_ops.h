/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*	Use of this software is restricted to the terms and
*	conditions of SerComm's software license agreement.
*
*			www.sercomm.com
****************************************************************/
#ifndef _WEB_BULK_OPS_H_
#define _WEB_BULK_OPS_H_

#define WEB_BULK_NAME	"WEB"

#define	WEB_MAX_PORT_NUM			65535
#define	WEB_MIN_PORT_NUM			1024

/* web_bulk_ops error code */
#define WEB_CONFLICT		-100
#define WEB_PORTINUSE		-101

struct WEB_DS{
	int	enable;
	int	sec_port;
	int 	ssport_enable;
	int 	ssport_number;
};


int WEB_BULK_ReadDS(void* ds);
int WEB_BULK_CheckDS(void* ds, void* ds_org);
int WEB_BULK_WriteDS(void* ds, void* ds_org);
int WEB_BULK_RunDS(void* ds, void* ds_org);
int WEB_BULK_WebMsg(int errcode, char* message, int* type);

#endif

