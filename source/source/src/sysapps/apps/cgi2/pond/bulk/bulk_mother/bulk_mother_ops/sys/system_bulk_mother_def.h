/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _SYSTEM_BULK_MOTHER_DEF_H_
#define  _SYSTEM_BULK_MOTHER_DEF_H_
#include <system_bulk_ops.h>
#include <system_bulk_mother.h>
struct bulk_mother_ops sys_bulk_mother_ops=
{
	name: SYS_BULK_NAME,
	fun: system_bulk_mother_fun,
};

#endif
