#ifndef _FTP_BULK_OPS_DEF_H_
#define _FTP_BULK_OPS_DEF_H_

#include <ftp_bulk_ops.h>
static struct FTP_DS ftp_ds;
static struct FTP_DS ftp_ds_org;
struct bulk_ops ftp_bulk_ops=
{
	name:		FTP_BULK_NAME,
	read: 		FTP_BULK_ReadDS,
 	check: 		FTP_BULK_CheckDS,
	write:		FTP_BULK_WriteDS,
	run:		NULL,
	web_msg:	FTP_BULK_WebMsg,
	ds:		&ftp_ds,
	ds_org: 	&ftp_ds_org,
	ds_size:	sizeof(struct FTP_DS),
	flag:		BULK_DEFAULT
};

#endif
