#ifndef _UPNP_BULK_OPS_DEF_H_
#define _UPNP_BULK_OPS_DEF_H_

#include <upnp_bulk_ops.h>
static struct UPNP_CONF_S upnp_ds;
static struct UPNP_CONF_S upnp_ds_org;
struct bulk_ops UPnP_bulk_ops=
{
        name:		UPnP_BULK_NAME,
        read:           UPnP_ReadConf,
        check:          UPnP_BULK_CheckDS,
        write:          UPnP_WriteConf,
        run:            UPnP_RunConf,
        ds:           	&upnp_ds,
        ds_org:       	&upnp_ds_org,
        ds_size:	sizeof(struct UPNP_CONF_S),
        flag:           BULK_DEFAULT
};

#endif
