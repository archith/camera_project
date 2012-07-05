#ifndef _RTSP_BULK_OPS_DEF_H_
#define _RTSP_BULK_OPS_DEF_H_

#include <rtsp_bulk_ops.h>
static struct rtsp_conf rtsp_ds;
static struct rtsp_conf rtsp_ds_org;
struct bulk_ops RTSP_bulk_ops=
{
        name:		RTSP_BULK_NAME,
        read:           RTSP_ReadConf,
        check:          RTSP_BULK_CheckDS,
        write:          RTSP_WriteConf,
        run:            RTSP_RunConf,
	web_msg:        RTSP_BULK_WebMsg,
        ds:           	&rtsp_ds,
        ds_org:       	&rtsp_ds_org,
        ds_size:		sizeof(struct rtsp_conf),
        flag:           BULK_DEFAULT
};

#endif
