/***************************************************************
 * (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
 *
 *      Use of this software is restricted to the terms and
 *      conditions of SerComm's software license agreement.
 *
 *                        www.sercomm.com
 ****************************************************************/
#ifndef _SCHD_FUNCTIONS_
#define _SCHD_FUNCTIONS_

#define SCHD_MAX_CMDLINE_LEN 128
#define SCHD_DONTCARE -1	/* don't care the time */

#define SCHD_OK        0
#define SCHD_ERROR    -1
#define SCHD_EINVALID -2	/* Invalid Input */
#define SCHD_EFILE    -3	/* conf file access error */
#define SCHD_EMEM     -4        /* out of memory */

typedef struct SchdTime {
	int interval;		/* schedule interval */
	int start;		/* schedule start time */
	int stop;		/* schedule stop time */
} SchdTime_t;

typedef struct SchdInfo {
	char command[SCHD_MAX_CMDLINE_LEN+1]; /* "command line" for linux */
	SchdTime_t week;	/* 0-6 (Sunday->Saturday) */
	SchdTime_t month;	/* 1-12 */
	SchdTime_t day;		/* 1-31 */
	SchdTime_t hour;	/* 0-23 */
	SchdTime_t min;		/* 0-59 */
} SchdInfo_t;

/* ---- !!!! !!!!                                                  ---- */
/* ---- If you don't need to set the time in any field, please set ---- */
/* ---- it to "SCHD_DONTCARE", or the function will set it to the  ---- */
/* ---- low layer program. ("0" is valid for some field!)          ---- */

/* ---- Add one entry to the schedule program (crond) ---- */
int SCHDAddSchedule(SchdInfo_t *info);
/* ---- Del one entry to the schedule program (crond) ---- */
int SCHDDelSchedule(SchdInfo_t *info);
/* Del "ALL" entries with the same "Command Line" argument to schedule 
 * program (crond). Only input "command" element is valid, the other will
 * discard.
 */
int SCHDDelScheduleByCmd(SchdInfo_t *info);
/* ---- Start the schedule program (crond) ---- */
int SCHDStartSchedule(void);
/* ---- Stop  the schedule program (crond) ---- */
int SCHDStopSchedule(void);

#endif
