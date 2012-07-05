
#ifndef __SC_LOCAL_TIME_H__
#define __SC_LOCAL_TIME_H__

//#define _GMT_TIME_TEST_
#include "sys/time.h"
/*
 * tz index, is the index value in the time-zone of UI.
 */
int sc_is_dst(int tz_index, int dstchanged);
int sc_change_dst(int tz_index, int is_adjust,int dsflag);
void sc_get_next_dst_change_time(struct tm *change_tm,int index_tz);
int sc_set_tz_name(int tz_index);
struct tm *localtime_sc(const time_t * const	timep);
time_t mktime_sc(struct tm * const	tmp);
int sc_gettimeofday(struct timeval *tv, struct timezone *tz);

#endif

