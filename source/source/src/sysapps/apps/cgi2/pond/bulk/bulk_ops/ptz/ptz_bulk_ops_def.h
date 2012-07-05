#ifndef _PTZ_BULK_OPS_DEF_H_
#define _PTZ_BULK_OPS_DEF_H_

#include <ptz_bulk_ops.h>
static struct PTZ_DS ptz_ds;
static struct PTZ_DS ptz_ds_org;
struct bulk_ops ptz_bulk_ops=
{
	name:		PTZ_BULK_NAME,
	read: 		PTZ_BULK_ReadDS,
 	check: 		PTZ_BULK_CheckDS,
	write:		PTZ_BULK_WriteDS,
	run:		PTZ_BULK_RunDS,
	ds:			&ptz_ds,
	ds_org: 	&ptz_ds_org,
	ds_size:	sizeof(struct PTZ_DS),
	flag:		BULK_DEFAULT
};

#endif
