#ifndef _SYSTEM_BULK_OPS_DEF_H_
#define _SYSTEM_BULK_OPS_DEF_H_

#include <system_bulk_ops.h>
static struct SYS_DS sys_ds;
static struct SYS_DS sys_ds_org;
struct bulk_ops sys_bulk_ops=
{
	name:		SYS_BULK_NAME,
	read: 		SYS_BULK_ReadDS,
 	check: 		SYS_BULK_CheckDS,
	write:		SYS_BULK_WriteDS,
	run:		SYS_BULK_RunDS,
	web_msg:	SYS_BULK_WebMsg,
	ds:			&sys_ds,
	ds_org: 	&sys_ds_org,
	ds_size:	sizeof(struct SYS_DS),
	flag:		BULK_DEFAULT
};


#endif
