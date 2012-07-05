/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _UPNP_BULK_OPS_H_
#define  _UPNP_BULK_OPS_H_

#define UPnP_BULK_NAME   		SEC_UPNP
#define UPNP_OK                         0
#define UPNP_READ_CONF_ERR              -1
#define UPNP_SAVE_CONF_ERR              -2
#define UPNP_START_ERR                  -3
#define UPNP_RESTART_ERR                -4
#define UPNP_STOP_ERR                   -5

typedef struct UPNP_CONF_S{
        int enable;
        int trv_enable;
        int security_enable;
}UPNP_CONF;

int StartUPnPDaemon(void);
int StopUPnPDaemon(void);
int StartTRVUPnPDaemon(void);
int StopTRVUPnPDaemon(void);
int RestartTRVUPnPDaemon(void);
int EndTRVUPnPDaemon(void);
int UPnP_ReadConf(void *ds);
int UPnP_BULK_CheckDS(void* ds, void* ds_org);
int UPnP_WriteConf(void *ds, void *ds_org);
int UPnP_RunConf(void *conf, void *conf_org);

#endif
