/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _BULK_MOTHER_H_
#define  _BULK_MOTHER_H_
#include <kdef.h>
#include <bulk_ops.h>
typedef int (*bulk_mother_run)(struct bulk_ops** bulk_ops_list);
int bulk_mother_ds(struct bulk_ops** bulk_ops_list);
int bulk_mother_funcat(int dummy, ...);
#endif

