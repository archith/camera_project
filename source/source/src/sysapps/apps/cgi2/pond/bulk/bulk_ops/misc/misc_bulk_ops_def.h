#ifndef _MISC_BULK_OPS_DEF_H_
#define _MISC_BULK_OPS_DEF_H_
#include <misc_bulk_ops.h>
static struct MISC_DS misc_ds;
static struct MISC_DS misc_ds_org;
struct bulk_ops misc_bulk_ops=
{
	name:		MISC_BULK_NAME,
	read: 		MISC_BULK_ReadDS,
 	check: 		MISC_BULK_CheckDS,
	write:		MISC_BULK_WriteDS,
	run:		MISC_BULK_RunDS,
	web_msg:	NULL,
	ds:		&misc_ds,
	ds_org: 	&misc_ds_org,
	ds_size:	sizeof(struct MISC_DS),
	flag:		BULK_DEFAULT
};

#endif
