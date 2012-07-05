#ifndef _QOS_BULK_OPS_DEF_H_
#define _QOS_BULK_OPS_DEF_H_

#include <qos_bulk_ops.h>
static struct QOS_DS qos_ds;
static struct QOS_DS qos_ds_org;
struct bulk_ops qos_bulk_ops=
{
	name:		QOS_BULK_NAME,
	read: 		QOS_BULK_ReadDS,
 	check: 		QOS_BULK_CheckDS,
	write:		QOS_BULK_WriteDS,
	run:		QOS_BULK_RunDS,
	ds:			&qos_ds,
	ds_org: 	&qos_ds_org,
	ds_size:	sizeof(struct QOS_DS),
	flag:		BULK_DEFAULT
};

#endif
