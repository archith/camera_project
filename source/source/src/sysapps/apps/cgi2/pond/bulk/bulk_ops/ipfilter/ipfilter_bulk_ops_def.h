#ifndef _IPFILTER_BULK_OPS_DEF_H_
#define _IPFILTER_BULK_OPS_DEF_H_

#include <ipfilter_bulk_ops.h>
static struct ipfilter_t IPFILTER_ds;
static struct ipfilter_t IPFILTER_ds_org;
struct bulk_ops IPFILTER_bulk_ops=
{
        name:		IPFILTER_BULK_NAME,
        read:           IPFILTER_ReadConf,
        check:          IPFILTER_BULK_CheckDS,
        write:          IPFILTER_WriteConf,
        run:            IPFILTER_RunConf,
        ds:           	&IPFILTER_ds,
        ds_org:       	&IPFILTER_ds_org,
        ds_size:	sizeof(struct ipfilter_t),
        flag:           BULK_DEFAULT
};

#endif
