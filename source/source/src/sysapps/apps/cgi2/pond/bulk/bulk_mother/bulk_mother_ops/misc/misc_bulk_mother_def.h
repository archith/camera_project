/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _MISC_BULK_MOTHER_DEF_H_
#define  _MISC_BULK_MOTHER_DEF_H_
#include <misc_bulk_ops.h>
#include <misc_bulk_mother.h>
struct bulk_mother_ops misc_bulk_mother_ops=
{
	name: MISC_BULK_NAME,
	fun: misc_bulk_mother_fun,
};

#endif
