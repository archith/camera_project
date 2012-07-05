/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef _BONJ_CONF_H
#define _BONJ_CONF_H

#define BONJ_BULK_NAME   SEC_BONJOUR
#define BONJNAME_LEN	32

typedef struct bonj_conf{
        int en;
	char bonjname[BONJNAME_LEN+1];
} bonj_conf;

int BONJ_start();
int BONJ_stop();
int BONJ_ReadConf(void *ds);
int BONJ_BULK_CheckDS(void* ds, void* ds_org);
int BONJ_WriteConf(void *ds, void *ds_org);
int BONJ_RunConf(void *conf, void *conf_org);

#endif
