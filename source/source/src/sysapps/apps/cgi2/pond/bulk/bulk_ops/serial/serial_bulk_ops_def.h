#ifndef _SERIAL_BULK_OPS_DEF_H_
#define _SERIAL_BULK_OPS_DEF_H_

#include <serial_bulk_ops.h>
static struct COM_DS com_ds;
static struct COM_DS com_ds_org;
struct bulk_ops com_bulk_ops=
{
	name:		COM_BULK_NAME,
	read: 		COM_BULK_ReadDS,
 	check: 		COM_BULK_CheckDS,
	write:		COM_BULK_WriteDS,
	run:		COM_BULK_RunDS,
	ds:			&com_ds,
	ds_org: 	&com_ds_org,
	ds_size:	sizeof(struct COM_DS),
	flag:		BULK_DEFAULT
};


#endif
