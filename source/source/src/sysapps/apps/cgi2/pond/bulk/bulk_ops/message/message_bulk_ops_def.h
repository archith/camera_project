#ifndef _MESSAGE_BULK_OPS_DEF_H_
#define _MESSAGE_BULK_OPS_DEF_H_

#include <message_bulk_ops.h>
static struct MESSAGE_DS message_ds={0, {'\0'}, {'\0'}};
struct bulk_ops message_bulk_ops=
{
	name:		MESSAGE_BULK_NAME,
	read: 		NULL,
 	check: 		NULL,
	write:		NULL,
	run:		NULL,
	ds:			&message_ds,
	ds_org: 	NULL,
	ds_size:	sizeof(struct MESSAGE_DS),
	flag:		BULK_DEFAULT
};


#endif
