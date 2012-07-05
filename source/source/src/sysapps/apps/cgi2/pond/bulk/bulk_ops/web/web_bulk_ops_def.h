#ifndef _WEB_BULK_OPS_DEF_H_
#define _WEB_BULK_OPS_DEF_H_

#include <web_bulk_ops.h>
static struct WEB_DS web_ds;
static struct WEB_DS web_ds_org;
struct bulk_ops web_bulk_ops=
{
	name:		WEB_BULK_NAME,
	read: 		WEB_BULK_ReadDS,
 	check: 		WEB_BULK_CheckDS,
	write:		WEB_BULK_WriteDS,
	run:		WEB_BULK_RunDS,
	web_msg:        WEB_BULK_WebMsg,
	ds:		&web_ds,
	ds_org: 	&web_ds_org,
	ds_size:	sizeof(struct WEB_DS),
	flag:		BULK_DEFAULT
};


#endif
