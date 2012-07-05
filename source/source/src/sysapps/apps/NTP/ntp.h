/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*                    All Rights Reserved.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/

#ifndef _GETNTP_H_
#define _GETNTP_H_

#include "model.h"
#include "conf_sec.h"
#include <log_api.h>

//#define _MY_DEBUG_
#define _ONEDAY_
#ifdef _MY_DEBUG_
#define MYPRINTF(format, argument...) printf(format , ## argument);
#else
#define MYPRINTF(format, argument...)
#endif 

/*------------------------------------------------------------------*/
/* Purpose: Get time from network time server then set to system    */
/* Input:   Three NTP server's ip address                  	    */
/* Return:  0 -Ok                                      		    */
/*          Other values  - error                      		    */
/*------------------------------------------------------------------*/
int NTP_update(char *serv);

#endif
