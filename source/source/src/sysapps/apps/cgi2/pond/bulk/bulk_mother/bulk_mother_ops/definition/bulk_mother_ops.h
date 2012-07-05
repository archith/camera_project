/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _BULK_MOTHER_OPS_H_
#define  _BULK_MOTHER_OPS_H_
#include <bulk_ops.h>
typedef int (*bulk_mother_fun)(struct bulk_ops *ops);
struct bulk_mother_ops
{
	char* name;
	bulk_mother_fun fun;
};

#endif
