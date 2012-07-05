/***************************************************************
 * (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
 *
 *      Use of this software is restricted to the terms and
 *      conditions of SerComm's software license agreement.
 *
 *                        www.sercomm.com
 ****************************************************************/
#ifndef MAF_MOT_H
#define MAF_MOT_H

#include "motypes.h"
#include "mot_bulk_ops.h"
//#include <IMG_conf.h>
#define	MP4BLKSIZE 	16  /*16x16*/ 	
#define	QLTY_NUM	5 
#define SENS_NUM	11
#define THREHOLD_NUM	11

#define TRGACTION	1
#define NO_TRGACTION	0

#define MOT_OK		0
#define MOT_FAIL	-1

extern S8 ClrIndicator(void);
S8 mot_setup();
void MOT_indicatorAct(int en);
#endif
