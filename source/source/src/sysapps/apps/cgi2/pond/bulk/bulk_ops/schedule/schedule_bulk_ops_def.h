#ifndef _SCHEDULE_BULK_OPS_DEF_H_
#define _SCHEDULE_BULK_OPS_DEF_H_

#include <schedule_bulk_ops.h>
static struct SCHEDULE_DS schedule_ds;
static struct SCHEDULE_DS schedule_ds_org;
struct bulk_ops schedule_bulk_ops=
{
	name:		SCHEDULE_BULK_NAME,
	read: 		SCHEDULE_BULK_ReadDS,
 	check: 		SCHEDULE_BULK_CheckDS,
	write:		SCHEDULE_BULK_WriteDS,
	run:		NULL,
	web_msg:	SCHEDULE_BULK_WebMsg,
	ds:			&schedule_ds,
	ds_org: 	&schedule_ds_org,
	ds_size:	sizeof(struct SCHEDULE_DS),
	flag:		BULK_DEFAULT
};


#endif
