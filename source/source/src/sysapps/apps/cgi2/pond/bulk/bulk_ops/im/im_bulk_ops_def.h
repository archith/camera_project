#ifndef _IM_BULK_OPS_DEF_H_
#define _IM_BULK_OPS_DEF_H_

#include <im_bulk_ops.h>
static struct IM_DS im_ds;
static struct IM_DS im_ds_org;
struct bulk_ops im_bulk_ops=
{
	name:		IM_BULK_NAME,
	read: 		IM_BULK_ReadDS,
	check: 		IM_BULK_CheckDS,
	write:		IM_BULK_WriteDS,
	run:		IM_BULK_RunDS,
	web_msg:	IM_BULK_WebMsg,
	ds:		&im_ds,
	ds_org: 	&im_ds_org,
	ds_size:	sizeof(struct IM_DS),
 	flag:		BULK_DEFAULT
};

#endif
