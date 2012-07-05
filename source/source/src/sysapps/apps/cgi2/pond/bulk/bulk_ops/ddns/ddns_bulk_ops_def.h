#ifndef _DDNS_BULK_OPS_DEF_H_
#define _DDNS_BULK_OPS_DEF_H_

#include <ddns_bulk_ops.h>
static struct ddns_param ddns_ds;
static struct ddns_param ddns_ds_org;
struct bulk_ops ddns_bulk_ops=
{
        name:		DDNS_BULK_NAME,
        read:           ddns_ReadConf,
        check:          ddns_BULK_CheckDS,
        write:          ddns_WriteConf,
        run:            ddns_RunConf,
        ds:           	&ddns_ds,
        ds_org:       	&ddns_ds_org,
        ds_size:	sizeof(struct ddns_param),
        flag:           BULK_DEFAULT
};

#endif
