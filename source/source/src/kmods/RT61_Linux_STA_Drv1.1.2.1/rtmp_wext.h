/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology	5th	Rd.
 * Science-based Industrial	Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2007, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************
 
    Module Name:
    rtmp_wext.h

    Abstract:
    This file was created for wpa_supplicant general wext driver support.

    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    Shiang      2007/04/03    Initial
    
*/

#ifndef __RTMP_WEXT_H__
#define __RTMP_WEXT_H__

#include <net/iw_handler.h>
#include "rt_config.h"


#define     WPARSNIE    0xdd
#define     WPA2RSNIE   0x30

#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT

int wext_notify_event_assoc(
	RTMP_ADAPTER *pAd, 
	USHORT iweCmd, 
	int assoc);

int wext2Rtmp_Security_Wrapper(
	RTMP_ADAPTER *pAd);

#endif

#endif // __RTMP_WEXT_H__

