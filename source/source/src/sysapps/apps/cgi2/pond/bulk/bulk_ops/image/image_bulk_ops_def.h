#ifndef _IMAGE_BULK_OPS_DEF_H_
#define _IMAGE_BULK_OPS_DEF_H_

#include <image_bulk_ops.h>
static struct IMAGE_DS image_ds;
static struct IMAGE_DS image_ds_org;
struct bulk_ops image_bulk_ops=
{
	name:	IMAGE_BULK_NAME,
	read: 	IMAGE_BULK_ReadDS,
 	check: 	IMAGE_BULK_CheckDS,
	write:	IMAGE_BULK_WriteDS,
	run:	IMAGE_BULK_RunDS,
	ds:		&image_ds,
	ds_org: 	&image_ds_org,
	ds_size:	sizeof(struct IMAGE_DS),
	flag:	BULK_DEFAULT
};

#endif
