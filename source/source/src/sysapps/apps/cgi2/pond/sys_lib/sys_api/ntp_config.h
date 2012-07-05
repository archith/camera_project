/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _NTP_CONFIG_H_
#define  _NTP_CONFIG_H_
#include "system_config.h"
#include "model.h"

#if (OEM != OEM_Linksys)
#define _USER_SETNTP_
#endif

#define  NTP_OK		 0
#define  NTP_ERROR	 -1

#define  NTP_ADDR_LEN		64	       /* XXX.XXX... */


typedef struct NTP_INFO {
	int status;		   /* 1:Enable, 0:Disable */
 	char server[NTP_ADDR_LEN+1];
	int  weekday;		  /* -1:Never 0:every day 1:Monday..7:Sunday */	
	int  hour;		   /* Hours (0-23) */	
	int  min;		   /* Minutes (0-59) */		
	int  port;		 /* 123,1024 - 65535 */
} NTP_INFO;

/*------------------------------------------------------------------*/
/* purpose: Read NTP setting 			   	            */
/* input:   NTP_INFO 		      			            */
/* return:  SYSTEM_OK - success					    */	
/*  	    SYSTEM_ERROR -failure				    */
/*------------------------------------------------------------------*/

int NTPReadData(NTP_INFO *ntpinfo);

/*------------------------------------------------------------------*/
/* purpose: Write NTP setting			   	            */
/* input:   NTP_INFO			    			    */
/* return:  SYSTEM_OK - success					    */	
/*  	    SYSTEM_ERROR -failure				    */
/*------------------------------------------------------------------*/

int NTPWriteData(NTP_INFO *ntpinfo);

/*------------------------------------------------------------------*/
/* purpose: Set NTP active now			   	            */
/* input:   void 		      			            */
/* return:  SYSTEM_OK - success					    */	
/*  	    SYSTEM_ERROR -failure				    */
/*------------------------------------------------------------------*/

int SetNTP(void);

/*------------------------------------------------------------------*/
/* purpose: Get time from NTP server			   	    */
/* input:   SYS_TIME						    */
/* return:  SYSTEM_OK - success					    */	
/*  	    other -failure				            */
/*------------------------------------------------------------------*/

int GetNTPTime(SYS_TIME *ntptime);

/*------------------------------------------------------------------*/
/* purpose: Read daylight saving  status			    */
/* input:   daysaving 						    */
/* return:  SYSTEM_OK - success					    */	
/*  	    SYSTEM_ERROR -failure				    */
/*------------------------------------------------------------------*/
int ReadDayLightSaving(int* daysaving);

/*------------------------------------------------------------------*/
/* purpose: Enable/Disable daylight saving			    */
/* input:   0-Disable 						    */
/*	    1-Enable						    */
/* return:  SYSTEM_OK - success					    */	
/*  	    SYSTEM_ERROR -failure				    */
/*------------------------------------------------------------------*/
int WriteDayLightSaving(int daysaving);

#endif	/* _NTP_CONFIG_H_ */
