#ifndef _CDP_BULK_OPS_DEF_H_
#define _CDP_BULK_OPS_DEF_H_

#include <cdp_bulk_ops.h>
static struct CDP_DS cdp_ds;
static struct CDP_DS cdp_ds_org;
struct bulk_ops cdp_bulk_ops=
{
	name:		CDP_BULK_NAME,
	read: 		CDP_BULK_ReadDS,
 	check: 		CDP_BULK_CheckDS,
	write:		CDP_BULK_WriteDS,
	run:		CDP_BULK_RunDS,
	web_msg:	CDP_BULK_WebMsg,
	ds:		&cdp_ds,
	ds_org: 	&cdp_ds_org,
	ds_size:	sizeof(struct CDP_DS),
	flag:		BULK_DEFAULT
};

#endif
