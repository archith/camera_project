#ifndef _AUDIO_BULK_OPS_DEF_H_
#define _AUDIO_BULK_OPS_DEF_H_

#include <audio_bulk_ops.h>
static struct AUDIO_DS audio_ds;
static struct AUDIO_DS audio_ds_org;
struct bulk_ops audio_bulk_ops=
{
	name:		AUDIO_BULK_NAME,
	read: 		AUDIO_BULK_ReadDS,
 	check: 		AUDIO_BULK_CheckDS,
	write:		AUDIO_BULK_WriteDS,
	run:		AUDIO_BULK_RunDS,
	web_msg:	NULL,
	ds:		&audio_ds,
	ds_org: 	&audio_ds_org,
	ds_size:	sizeof(struct AUDIO_DS),
	flag:		BULK_DEFAULT
};

#endif
