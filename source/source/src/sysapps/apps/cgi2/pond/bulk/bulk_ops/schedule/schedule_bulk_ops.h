/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*       Use of this software is restricted to the terms and
*       conditions of SerComm's software license agreement.
*
*                       www.sercomm.com
****************************************************************/
#ifndef _SCHEDULE_BULK_OPS_H_
#define _SCHEDULE_BULK_OPS_H_
#include <bulk_ops.h>

#define SCHEDULE_BULK_NAME	"SCHEDULE"


#define SCHEDULE_NUM	10

#define ADD_SCHEDULE	1
#define DEL_SCHEDULE	2

#define SCHEDULE_NONE	0
#define SCHEDULE_MON	1
#define SCHEDULE_TUE	2
#define SCHEDULE_WED	3
#define SCHEDULE_THU	4
#define SCHEDULE_FRI	5
#define SCHEDULE_SAT	6
#define SCHEDULE_SUN	7
#define SCHEDULE_WDAY	8
#define SCHEDULE_ALL	9

/* schedule error code */
#define	SCH_EINTERVAL	-100
#define SCH_WADDSCH	(BULK_OPS_MESSAGE_FLAG | 2)
#define SCH_WDELSCH	(BULK_OPS_MESSAGE_FLAG | 3)

/* BULK */
typedef struct{
	int day;	// 0: emply, 1~7: Mon~Sun, 8: Weekday, 9: everyday
	int s_hr;	// start time(24hrs)
	int s_min;
	int e_hr;	// end time(24hrs)
	int e_min;
}SCHEDULE_TAB;

struct SCHEDULE_DS{
	int enable;
	SCHEDULE_TAB table[SCHEDULE_NUM];
	int action;	// 0-none 1-add 2-del
	SCHEDULE_TAB change;
};


int SCHEDULE_BULK_ReadDS(void* ds);
int SCHEDULE_BULK_CheckDS(void* ds, void* ds_org);
int SCHEDULE_BULK_WriteDS(void* ds, void* ds_org);
int SCHEDULE_BULK_WebMsg(int errcode, char* message, int* type);
#endif
