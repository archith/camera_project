#ifndef _EVENT_BULK_OPS_DEF_H_
#define _EVENT_BULK_OPS_DEF_H_

#include <event_bulk_ops.h>
static struct EVENT_DS event_ds;
static struct EVENT_DS event_ds_org;
struct bulk_ops event_bulk_ops=
{
	name:		EVENT_BULK_NAME,
	read: 		EVENT_BULK_ReadDS,
 	check: 		EVENT_BULK_CheckDS,
	write:		EVENT_BULK_WriteDS,
	run:		EVENT_BULK_RunDS,
	ds:			&event_ds,
	ds_org: 	&event_ds_org,
	ds_size:	sizeof(struct EVENT_DS),
	flag:		BULK_DEFAULT
};


#endif
