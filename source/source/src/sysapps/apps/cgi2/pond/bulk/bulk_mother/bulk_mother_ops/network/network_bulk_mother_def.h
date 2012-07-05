/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _NETWORK_BULK_MOTHER_DEF_H_
#define  _NETWORK_BULK_MOTHER_DEF_H_
#include <network_bulk_ops.h>
#include <network_bulk_mother.h>
struct bulk_mother_ops network_bulk_mother_ops=
{
	name: NET_BULK_NAME,
	fun: network_bulk_mother_fun,
};

#endif
