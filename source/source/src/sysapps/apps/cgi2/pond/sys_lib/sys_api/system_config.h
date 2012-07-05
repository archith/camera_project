/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _SYSTEM_CONFIG_H_
#define  _SYSTEM_CONFIG_H_

#define  SYSTEM_OK		 0
#define  SYSTEM_ERROR	        -1

#define  DEV_LEN		16      
#define  DES_LEN		32
#define  HW_LEN			17	       /* 00:C0:02:12:34:56 */

//#define  ADDR_LEN		49	       /* XXX.XXX... */
#define  DATE_LEN		20	       /* XX/XX/XXXX */

#define  VER_LEN        2              	/* AB CD - > A.BCR0D */

#define DEF_DATE_TYPE	2 		/* 1: YYYY-MM-DD */
					/* 2: MM/DD/YYYY */
					/* 3: DD/MM/YYYY */
#define SERIALNO_LEN  	12

typedef struct SYS_INFO {
        char device[DEV_LEN+1];                /* Device Name */
        char description[DES_LEN+1];           /* Description/Location */
	int led_operation;			       /* Led Op Control */
} SYS_INFO;


typedef struct HW_INFO {
   	char version[VER_LEN];		       	       /* F/W Version */
	char mac_addr[HW_LEN+1];		       /* MAC Address  */
	char def_name[DEV_LEN+1];		       /* Default Sever name */
	char release_date[DATE_LEN+1];		       /* Release date */
	char serialno[SERIALNO_LEN+1];				/*serial number*/
} HW_INFO;


typedef struct SYS_TIME {
   int sys_month;		   /* The month of system date   (1-12) */
   int sys_day;			   /* The day of system date     (1-31) */	
   int sys_year;                   /* The year of system date */
   int sys_hour;                   /* The hour of system date    (0-23) */
   int sys_min;                    /* The minutes of system date (0-59) */
   int sys_sec;                    /* The seconds of system date (0-59) */
   float adj_time;		   /* adjust time of system date */
} SYS_TIME;		                      

typedef struct TIME_FMT {
   int datetype;
   int timetype;
   int t_index;		   	   /* The index of time zone */
   int d_sav;			   /* DayLight saving */
} TIME_FMT;

/*------------------------------------------------------------------*/
/* purpose: Read Device data From the configuration File            */
/* input:   SYS_INFO						    */
/* return:  SYSTEM_OK - success					    */	
/*  	    SYSTEM_ERROR -failure				    */
/*------------------------------------------------------------------*/

int SYSINFO_ReadConfigData(SYS_INFO *sysinfo);

/*------------------------------------------------------------------*/
/* purpose: Write Device data to the configuration File             */
/* input:   SYS_INFO						    */
/* return:  SYSTEM_OK - success					    */	
/*  	    SYSTEM_ERROR -failure				    */
/*------------------------------------------------------------------*/

int SYSINFO_WriteConfigData(SYS_INFO *sysinfo);

/*------------------------------------------------------------------*/
/* purpose: Read MAC Address and F/W version From the system        */
/* input:   HW_INFO						    */
/* return:  SYSTEM_OK - success					    */	
/*  	    SYSTEM_ERROR -failure				    */
/*------------------------------------------------------------------*/

int HWINFO_ReadConfigData(HW_INFO *hwinfo);

/*------------------------------------------------------------------*/
/* purpose: Read time data from the system                          */
/* input:   SYS_TIME						    */
/* return:  SYSTEM_OK - success					    */	
/*  	    SYSTEM_ERROR -failure				    */
/*------------------------------------------------------------------*/

int TIME_ReadSystemData(SYS_TIME *timeinfo);

/*------------------------------------------------------------------*/
/* purpose: Write time data to the system                           */
/* input:   SYS_TIME						    */
/* return:  SYSTEM_OK - success					    */	
/*  	    SYSTEM_ERROR -failure				    */
/*------------------------------------------------------------------*/

int TIME_WriteSystemData(SYS_TIME *timeinfo);

/*------------------------------------------------------------------*/
/* purpose: Check input date and time is valid or not               */
/* input:   SYSTIME 						    */
/* return:  SYSTEM_OK -  valid					    */	
/*  	    SYSTEM_ERROR - invalid 				    */
/*------------------------------------------------------------------*/
int Check_date_time(SYS_TIME *timeinfo);

/*------------------------------------------------------------------*/
/* purpose: Read time relative format  from the system              */
/* input:   TIME_FMT						    */
/* return:  SYSTEM_OK - success					    */	
/*  	    SYSTEM_ERROR -failure				    */
/*------------------------------------------------------------------*/

int TIME_FMT_ReadSystemData(TIME_FMT *timefmt);

/*------------------------------------------------------------------*/
/* purpose: Write time relative format to the system                */
/* input:   TIME_FMT						    */
/* return:  SYSTEM_OK - success					    */	
/*  	    SYSTEM_ERROR -failure				    */
/*------------------------------------------------------------------*/

int TIME_FMT_WriteSystemData(TIME_FMT *timefmt);


#endif	/* _SYSTEM_CONFIG_H_ */
