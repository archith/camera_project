#ifndef _WLAN_BULK_OPS_DEF_H_
#define _WLAN_BULK_OPS_DEF_H_

#include <wlan_bulk_ops.h>
static struct WLAN_DS wlan_ds;
static struct WLAN_DS wlan_ds_org;
struct bulk_ops wlan_bulk_ops=
{
	name:		WLAN_BULK_NAME,
	read: 		WLAN_BULK_ReadDS,
 	check: 		WLAN_BULK_CheckDS,
	write:		WLAN_BULK_WriteDS,
	run:		WLAN_BULK_RunDS,
	web_msg:	WLAN_BULK_WebMsg,
	ds:			&wlan_ds,
	ds_org: 	&wlan_ds_org,
	ds_size:	sizeof(struct WLAN_DS),
	flag:		BULK_DEFAULT
};

#endif
