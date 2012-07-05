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
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <net/route.h>
#include <profile.h>
#include "../inc/SCHD_functions.h"
#include "../inc/system_config.h"

#define NTP_SETCRON	   "ntp_setcron"
#define NTP_BINARY	   "ntpdate"
#define EXEC_NTP_BINARY	   "/usr/local/bin/ntpdate"
