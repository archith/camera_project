#ifndef _LOG_BULK_OPS_DEF_H_
#define _LOG_BULK_OPS_DEF_H_

#include <log_bulk_ops.h>
static struct LOG_DS log_ds;
static struct LOG_DS log_ds_org;
struct bulk_ops log_bulk_ops=
{
	name:		LOG_BULK_NAME,
	read: 		LOG_BULK_ReadDS,
	check: 		LOG_BULK_CheckDS,	
	write:		LOG_BULK_WriteDS,
	run:		LOG_BULK_RunDS,
	ds:			&log_ds,
	ds_org: 	&log_ds_org,
	ds_size:	sizeof(struct LOG_DS),
 	flag:		BULK_DEFAULT
};


#endif
