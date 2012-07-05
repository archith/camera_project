#ifndef _SMTP_BULK_OPS_DEF_H_
#define _SMTP_BULK_OPS_DEF_H_

#include <smtp_bulk_ops.h>
static struct SMTP_DS smtp_ds;
static struct SMTP_DS smtp_ds_org;
struct bulk_ops smtp_bulk_ops=
{
	name:		SMTP_BULK_NAME,
	read: 		SMTP_BULK_ReadDS,
 	check: 		SMTP_BULK_CheckDS,
	write:		SMTP_BULK_WriteDS,
	run:		NULL,
	web_msg:	SMTP_BULK_WebMsg,
	ds:			&smtp_ds,
	ds_org: 	&smtp_ds_org,
	ds_size:	sizeof(struct SMTP_DS),
	flag:		BULK_DEFAULT
};

#endif
