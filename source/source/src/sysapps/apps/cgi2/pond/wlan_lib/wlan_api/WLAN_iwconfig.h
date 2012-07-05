/***************************************************************
 * (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
 *
 *      Use of this software is restricted to the terms and
 *      conditions of SerComm's software license agreement.
 *
 *                        www.sercomm.com
 ****************************************************************/
#ifndef  _WLAN_IWCONFIG_H_
#define  _WLAN_IWCONFIG_H_

#include "conf_sec.h"

#define CONF_FILE_WLAN		CONF_FILE
				/* section name */
#define CONF_SECT_WLAN		SEC_WLAN
				/* keys */
#define CONF_KEY_OPMODE		WLAN_TYPE
#define CONF_KEY_ESSID		WLAN_ESSID
#define CONF_KEY_CHANNEL	WLAN_CHANNEL
#ifdef _WLAN_DOMAIN_
#define CONF_KEY_DOMAIN		WLAN_DOMAIN	/* Vincent_001 */
#endif /* _WLAN_DOMAIN_ */
#define CONF_KEY_WEPMODE	WLAN_WEP_MODE
#define CONF_KEY_WEPAUTHTYPE	WLAN_AUTH
#define CONF_KEY_WEP_INDEX	WLAN_WEP_INDEX
#define CONF_KEY_WEP_KEY1	WLAN_WEP_KEY1
#define CONF_KEY_WEP_KEY2	WLAN_WEP_KEY2
#define CONF_KEY_WEP_KEY3	WLAN_WEP_KEY3
#define CONF_KEY_WEP_KEY4	WLAN_WEP_KEY4
#define CONF_KEY_ESSID_ANY	"ANY"	/* ANY ESSID (case-insensitive) */
#define CONF_KEY_SECURITY	WLAN_SECURITY
#define CONF_KEY_PSK_STR	WLAN_WPA_KEY
// Temproary using
#define CONF_KEY_PSK_KEY	MAN_PSK_KEY
#define CONF_WMM		WLAN_WMM
#define CONF_KEY_WPA_AUTH	WLAN_WPA_AUTHTYPE
#define CONF_KEY_TLS_USER	WLAN_TLSUSER
#define CONF_KEY_TLS_KEYPASSWD	WLAN_TLSKEYPASSWD
#define CONF_KEY_TTLS_AUTH	WLAN_TTLSAUTHTYPE
#define CONF_KEY_TTLS_USER	WLAN_TTLSUSER
#define CONF_KEY_TTLS_PASSWD	WLAN_TTLSPASSWD
#define CONF_KEY_TTLS_ANONY	WLAN_TTLSANONYNAME
#define CONF_KEY_PEAP_AUTH	WLAN_PEAPAUTHTYPE
#define CONF_KEY_PEAP_USER	WLAN_PEAPUSER
#define CONF_KEY_PEAP_PASSWD	WLAN_PEAPPASSWD
#define CONF_KEY_PEAP_ANONY	WLAN_PEAPANONYNAME


#define CONF_RDWR_OK		0	/* return OK for profile library */

				/* Vincent_001: Domain Settings: Start */
#if 0
#ifdef _WLAN_DOMAIN_
/* MAX channel=14, use 16bit: 0x0000 ~ 0x3FFF: (Japan: 0010:0111 1111:1111) */
const unsigned short accept_channel[WLAN_DOMAIN_NUM] = {
	0x1FFF,			/* Africa:         1-13 */
	0x1FFF,			/* Asia:           1-13 */
	0x1FFF,			/* Australia:      1-13 */
	0x07FF,			/* Canada:         1-11 */
	0x1FFF,			/* Europe:         1-13 */
	0x0600,			/* Spain:         10-11 */
	0x1E00,			/* France:        10-13 */
	0x0070,			/* Israel:         5-7  */
	0x3FFF,			/* Japan:          1-14 */
	0x0400,			/* Mexico:        11-11 */
	0x1FFF,			/* South America:  1-13 */
	0x07FF,			/* USA:            1-11 */
};
#endif /* _WLAN_DOMAIN_ */
				/* Vincent_001: Domain Settings: Stop  */

		/* The following 2 items MUST be the same as "orinoco.c"! */
const long channel_frequency[] = {
	2412, 2417, 2422, 2427, 2432, 2437, 2442,
	2447, 2452, 2457, 2462, 2467, 2472, 2484
};
#define NUM_CHANNELS ( sizeof(channel_frequency) / sizeof(channel_frequency[0]) )
		/* ++++ End-of-Channel-Frequenc ++++ */

/* -- Preventing some ethernet interface return OK when do "ioctl" -- */
#define WIRELESS_MAGIC "IEEE 802.11-DS"
#endif

/**************************** VARIABLES ****************************/
/* We won't use the following strings (from iwconfig), but we need to know the
 * order. (TOTAL=6, Seq No.: 0=Auto, 1=Ad-Hoc, 2=Managed ...)
 */
/*
char *	operation_mode[] = { "Auto", "Ad-Hoc", "Managed", "Master", "Repeater", "Secondary" };
 */

/* Copy from "sys/socket.h".                  Vincent Kuo 2002/07/31 *
 * Due to too many duplication with "wireless.h, I copy 1 line only. */
/* Create a new socket of type TYPE in domain DOMAIN, using
   protocol PROTOCOL.  If PROTOCOL is zero, one is chosen automatically.
   Returns a file descriptor for the new socket, or -1 for errors.  */
extern int socket __P ((int __domain, int __type, int __protocol));

#ifdef _WLAN_DEBUG_
#include <stdio.h>		/* printf */
#include <errno.h>
#define dbugprintf(format, argument...) printf(format, ## argument);
#else
#define dbugprintf(format, argument...)
#endif

#endif	/* _WLAN_IWCONFIG_H_ */

