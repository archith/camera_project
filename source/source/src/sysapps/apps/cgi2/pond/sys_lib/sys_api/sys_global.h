/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <net/route.h>
#include "model.h"
#include <profile.h>
//#include "../inc/PRO_file.h"
#include "../inc/SCHD_functions.h"

#if 0
#define DEF_SYSTEM_CONF    "/etc/def_sys.conf"
#define SYSTEM_CONF   	   "/etc/system.conf"
#define CGI_SECTION	   "system"   
#define CGI_HOSTNAME       "hostname"
#define CGI_DESCRIPTION    "comment"
#define CGI_HWADDR 	   "hw_addr"
#define CGI_LEDOP	   "led_operation"
#define CGI_DAYLIGHTSAVING "day_saving"
#define CGI_DATETYPE	   "date_type"
#define CGI_TINDEX	   "t_index"
#define CGI_VERSION 	   "version"
#define CGI_RELEASE	   "release_date"
#define CGI_NTP		   "ntp"
#define CGI_NTPSERVER      "ntp_server"
#define CGI_NTPWEEKDAY	   "ntp_weekday"
#define CGI_NTPHOUR	   "ntp_hour"
#define CGI_NTPMIN	   "ntp_min"
#endif
#if (OEM == OEM_Linksys)
#define _LINKSYS_
#elif (OEM ==OEM_Planet)
#define _PLANET_
#elif (OEM == OEM_Allnet)
#define _ALLNET_
#elif (OEM == OEM_Siemens)
#define _SIEMENS
#elif (OEM == OEM_Motorola)
#define _MOTOROLA_
#elif (OEM == OEM_Corega)
#define _COREGA_
#endif
#define BufferSize	   1024
#define PID_FILE	   "/etc/PID"
#define PID_SIZE	   70
#define FWID_POS	   57
#define NTP_BINARY	   "ntpdate"
#define EXEC_NTP_BINARY	   "/usr/local/bin/ntpdate"
#define STR_NULL_IP	   "0.0.0.0"
