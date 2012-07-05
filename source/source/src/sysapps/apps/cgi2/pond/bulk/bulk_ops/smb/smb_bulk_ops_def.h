#ifndef _SMB_BULK_OPS_DEF_H_
#define _SMB_BULK_OPS_DEF_H_

#include <smb_bulk_ops.h>
static struct SMB_DS smb_ds;
static struct SMB_DS smb_ds_org;
struct bulk_ops smb_bulk_ops=
{
	name:		SMB_BULK_NAME,
	read: 		SMB_BULK_ReadDS,
	check: 		SMB_BULK_CheckDS,
	write:		SMB_BULK_WriteDS,
	run:		SMB_BULK_RunDS,
	web_msg:	NULL,
	ds:		&smb_ds,
	ds_org: 	&smb_ds_org,
	ds_size:	sizeof(struct SMB_DS),
 	flag:		BULK_DEFAULT
};

#endif
