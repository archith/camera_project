/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _SYSTEM_BILK_OPS_H_
#define  _SYSTEM_BILK_OPS_H_

#include <system_config.h>
#include <ntp_config.h>
#define SYS_BULK_NAME		"SYSTEM"

struct SYS_DS
{
	char host[DEV_LEN+1];
 	char comment[DES_LEN+1];
	char def_host[DEV_LEN+1];
   	char ver[VER_LEN];		       	       /* F/W Version */	
	char mac_addr[HW_LEN+1];		       /* MAC Address  */
	char serialno[SERIALNO_LEN+1];
	int  sys_month;
	int  sys_day;
	int  sys_year;
	int  sys_hour;
	int  sys_min;
	int  sys_sec;
	int  time_fmt;	
	int  date_fmt;
	int  tz;
	int  day_sav;
	int  ntp_en;
	char ntp_srv[NTP_ADDR_LEN+1];
	int  ntp_wday;
	int  ntp_hr;
	int  ntp_min;
	int  ntp_port;
	int  led_en;
};

int SYS_BULK_ReadDS(void* ds);
int SYS_BULK_CheckDS(void* ds, void* ds_org);
int SYS_BULK_WriteDS(void* ds, void* ds_org);
int SYS_BULK_RunDS(void* ds, void* ds_org);
int SYS_BULK_WebMsg(int errcode, char* message, int* type);

#endif

