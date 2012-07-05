/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _IPFILTER_BULK_OPS_H_
#define  _IPFILTER_BULK_OPS_H_

#define IPFILTER_BULK_NAME   		SEC_IPFILTER
#define IPFILTER_OK		0
#define IPFILTER_READ_ERR	-1
#define IPFILTER_SAVE_ERR	-2
#define IPFILTER_RESTART_ERR	-3

#define MAX_RULE_NUM	20

typedef struct ipfilter_t
{
	int ipfilter_enable;
	int ipfilter_policy;	// 1: deny,  0: accept
	char ipfilter_range[MAX_RULE_NUM][64];
}ipfilter_t;

int IPFILTER_ReadConf(void *ds);
int IPFILTER_BULK_CheckDS(void* ds, void* ds_org);
int IPFILTER_WriteConf(void *ds, void *ds_org);
int IPFILTER_RunConf(void *conf, void *conf_org);

#endif
