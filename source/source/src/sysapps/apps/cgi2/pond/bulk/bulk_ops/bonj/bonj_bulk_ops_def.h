#ifndef _BONJ_BULK_OPS_DEF_H_
#define _BONJ_BULK_OPS_DEF_H_

#include <bonj_bulk_ops.h>
static struct bonj_conf bonj_ds;
static struct bonj_conf bonj_ds_org;
struct bulk_ops bonj_bulk_ops=
{
        name:		BONJ_BULK_NAME,
        read:           BONJ_ReadConf,
        check:          BONJ_BULK_CheckDS,
        write:          BONJ_WriteConf,
        run:            BONJ_RunConf,
        web_msg:	NULL,
        ds:           	&bonj_ds,
        ds_org:       	&bonj_ds_org,
        ds_size:	sizeof(struct bonj_conf),
        flag:           BULK_DEFAULT
};


#endif
