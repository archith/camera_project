/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _RTSP_BULK_MOTHER_DEF_H_
#define  _RTSP_BULK_MOTHER_DEF_H_
#include <rtsp_bulk_ops.h>
#include <rtsp_bulk_mother.h>
struct bulk_mother_ops rtsp_bulk_mother_ops=
{
	name: RTSP_BULK_NAME,
	fun: rtsp_bulk_mother_fun,
};

#endif
