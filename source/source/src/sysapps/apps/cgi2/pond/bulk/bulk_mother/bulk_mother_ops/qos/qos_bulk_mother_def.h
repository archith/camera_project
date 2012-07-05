/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _QOS_BULK_MOTHER_DEF_H_
#define  _QOS_BULK_MOTHER_DEF_H_
#include <qos_bulk_ops.h>
#include <qos_bulk_mother.h>
struct bulk_mother_ops qos_bulk_mother_ops=
{
	name: QOS_BULK_NAME,
	fun: qos_bulk_mother_fun,
};

#endif
