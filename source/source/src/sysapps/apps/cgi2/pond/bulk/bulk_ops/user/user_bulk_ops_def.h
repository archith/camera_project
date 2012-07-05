#ifndef _USER_BULK_OPS_DEF_H_
#define _USER_BULK_OPS_DEF_H_

#include <user_bulk_ops.h>
static struct USER_DS user_ds;
static struct USER_DS user_ds_org;
struct bulk_ops user_bulk_ops=
{
	name:		USER_BULK_NAME,
	read: 		USER_BULK_ReadDS,
 	check: 		USER_BULK_CheckDS,
	write:		USER_BULK_WriteDS,
	run:		USER_BULK_RunDS,
	web_msg:	USER_BULK_WebMsg,
	ds:		&user_ds,
	ds_org: 	&user_ds_org,
	ds_size:	sizeof(struct USER_DS),
	flag:		BULK_DEFAULT
};

#endif
