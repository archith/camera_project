#ifndef _NETWORK_BULK_OPS_DEF_H_
#define _NETWORK_BULK_OPS_DEF_H_

#include <network_bulk_ops.h>
static struct NET_DS net_ds;
static struct NET_DS net_ds_org;
struct bulk_ops net_bulk_ops=
{
	name:		NET_BULK_NAME,
	read: 		NET_BULK_ReadDS,
 	check: 		NET_BULK_CheckDS,
	write:		NET_BULK_WriteDS,
	run:		NET_BULK_RunDS,
	web_msg:	NET_BULK_WebMsg,
	ds:			&net_ds,
	ds_org: 	&net_ds_org,
	ds_size:	sizeof(struct NET_DS),
	flag:		BULK_DEFAULT
};

#endif
