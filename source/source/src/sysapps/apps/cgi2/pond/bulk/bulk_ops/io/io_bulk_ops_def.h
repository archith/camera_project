#ifndef _IO_BULK_OPS_DEF_H_
#define _IO_BULK_OPS_DEF_H_

#include <io_bulk_ops.h>
static struct IO_DS io_ds;
static struct IO_DS io_ds_org;
struct bulk_ops io_bulk_ops=
{
	name:		IO_BULK_NAME,
	read: 		IO_BULK_ReadDS,
 	check: 		IO_BULK_CheckDS,
	write:		IO_BULK_WriteDS,
	run:		IO_BULK_RunDS,
	web_msg:	IO_BULK_WebMsg,
	ds:		&io_ds,
	ds_org: 	&io_ds_org,
	ds_size:	sizeof(struct IO_DS),
	flag:		BULK_DEFAULT
};

#endif
