#ifndef _DNSWITCH_BULK_OPS_DEF_H_
#define _DNSWITCH_BULK_OPS_DEF_H_

#include <dnswitch_bulk_ops.h>
static struct DNSWITCH_DS dnswitch_ds;
static struct DNSWITCH_DS dnswitch_ds_org;
struct bulk_ops dnswitch_bulk_ops=
{
	name:		DNSWITCH_BULK_NAME,
	read: 		DNSWITCH_BULK_ReadDS,
 	check: 		DNSWITCH_BULK_CheckDS,
	write:		DNSWITCH_BULK_WriteDS,
	run:		DNSWITCH_BULK_RunDS,
	ds:		&dnswitch_ds,
	ds_org: 	&dnswitch_ds_org,
	ds_size:	sizeof(struct DNSWITCH_DS),
	flag:		BULK_DEFAULT
};

#endif
