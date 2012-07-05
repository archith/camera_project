/***************************************************************
 * (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
 *
 *      Use of this software is restricted to the terms and
 *      conditions of SerComm's software license agreement.
 *
 *                        www.sercomm.com
 ****************************************************************/

#include  <time.h>
#include  <unistd.h>
#include "ntp_config.h"
#include "SCHD_functions.h"
#include "model.h"
#if (OEM != OEM_Linksys)
#define _USER_SETNTP_
#endif
#define  NTP_SETCRON	"ntp_setcron"

int main(int argc,char **argv){
	int ret;
        time_t now;
	struct tm *now_tm;
	NTP_INFO ntp;

	SchdInfo_t schinfo={ {NTP_SETCRON},
			 {SCHD_DONTCARE, SCHD_DONTCARE, SCHD_DONTCARE},
			 {SCHD_DONTCARE, SCHD_DONTCARE, SCHD_DONTCARE},
			 {SCHD_DONTCARE, SCHD_DONTCARE, SCHD_DONTCARE},
			 {SCHD_DONTCARE, SCHD_DONTCARE, SCHD_DONTCARE},
			 {SCHD_DONTCARE, SCHD_DONTCARE, SCHD_DONTCARE},
			};

#ifndef _USER_SETNTP_
	if (1 == argc)
	{
		SetNTP();
		return NTP_OK;
	}
#endif

	ret=SCHDDelScheduleByCmd(&schinfo);
	
	if (ret != NTP_OK){
		return NTP_ERROR;
	}

	NTPReadData(&ntp);
#ifdef _USER_SETNTP_	
 	SetNTP();	
#endif
	time(&now);
	now_tm=localtime(&now);

#ifndef _USER_SETNTP_	// Make sure the NTP will not run again
	now -= 61;
	now_tm=localtime(&now);
	schinfo.hour.start = now_tm->tm_hour;
	schinfo.min.start = now_tm->tm_min;
#else
	//Prevent re-execute on crontab
	if((now_tm->tm_hour == ntp.hour ) && (now_tm->tm_min == ntp.min))
		sleep(61);

	schinfo.hour.start = ntp.hour;
	schinfo.min.start = ntp.min;
#endif

	ret=SCHDAddSchedule(&schinfo);

	if(ret != NTP_OK){
		return NTP_ERROR;
	}

	return NTP_OK;
}
