/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2005.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
//  0   1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
// |--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|
//   year  week  year month  day   hour  min   sec
//  20(?) 00-06 00-99 01-12 01-31 00-23 00-59 00-59

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;


int rtc_reset (void);
int set_RTC_time(unsigned char *time);
int read_RTC_time (unsigned char *time);
