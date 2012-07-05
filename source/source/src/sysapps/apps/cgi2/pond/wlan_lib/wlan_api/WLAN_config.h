/***************************************************************
 * (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
 *
 *      Use of this software is restricted to the terms and
 *      conditions of SerComm's software license agreement.
 *
 *                        www.sercomm.com
 ****************************************************************/
#ifndef  _WLAN_CONFIG_H_
#define  _WLAN_CONFIG_H_
#include <string.h>
#include <profile.h>		/* our own profile API */


#define _WLAN_DOMAIN_
#define _SC_WIRELESS_WEPKEY_HAS_ASCII_MODE_

#define WLAN_OK			0	 /* Action Complete */
#define WLAN_ERROR		-1	 /* Action Failure */
				/* Vincent_002: Block Start */
#define WLAN_ESOCKET		-2	 /* Open Socket Failure */
#define WLAN_EFILE		-3	 /* Open/Read/Write File Failure */
#define WLAN_EDEVICE		-4	 /* There's no wireless extensions on all network devices */
#define WLAN_EINVALID		-5	 /* Invalid Value (INPUT) */
#define WLAN_ESETMODE		-6	 /* Set Operation Mode Failure */
#define WLAN_ESETESSID		-7	 /* Set ESSID Failure */
#define WLAN_ESETCHANNEL	-8	 /* Set Channel Failure */
#define WLAN_ESETWEPKEY		-9	 /* Set WEP-KEY Failure */
#define WLAN_ESETDOMAIN		-10	 /* Set Domain Failure */
				/* Vincent_002: Block Stop  */
#define ATHEROS			"/proc/athxxx"	 
#define RALINK_CARD	 	2661
#define ATH_CARD		2414

#define WLAN_MODE_ADHOC		0
#define WLAN_MODE_INFRA		1
#define WLAN_MODE_UNKNOWN	-1 /* Vincent_002 */
#define WLAN_ESSID_ANY		0 /* if (strlen(essid)==0) { ANY!! } */
#define WLAN_CHANNEL_INVALID	0 /* Vincent_002 */
#define WLAN_WEP_DISABLE	0
#define WLAN_WEP_64BIT		1
#define WLAN_WEP_128BIT		2
#ifdef _SC_WIRELESS_WEPKEY_HAS_ASCII_MODE_
#define WLAN_WEP_64BITa		3
#define WLAN_WEP_128BITa	4
#endif
#define WLAN_WEP_AUTH_OPEN	1	/* Authentication Type: Open System */
#define WLAN_WEP_AUTH_SHARE	2	/* Authentication Type: Shared Key  */
						/* Vincent_001: Block Start */
#define WLAN_WEP_KEYS		4	/* 4 keys */
#define WLAN_WEP_64KEY_SIZE	5	/* 5  bytes per key ( 5x8= 40 bit) */
#define WLAN_WEP_128KEY_SIZE	13	/* 13 bytes per key (13x8=104 bit) */
						/* Vincent_001: Block Stop  */
//#define WLAN_MAX_ESSID_LEN	32	/* Linux supports up to 34 char */
#define WLAN_MAX_ESSID_LEN	34	/* Linux supports up to 34 char */
#define WLAN_MAX_WEP_KEY	32	/* String Format Length */	/* Vincent_001: Modify */

#define WLAN_PSK_KEY_LEN	32
#define WLAN_MAX_PSK_PASS_STR   63
#define PSK_FILE		"/etc/wpa_supplicant.conf"
#define RT61STA_DATA		"/mnt/ramdisk/rt61sta.dat"
						/* Vincent_006: Block Start */
#define WLAN_NONE		0
#define WLAN_WEP		1
#define WLAN_WPA_PERSONAL	2
#define WLAN_WPA_ENTERPRISE	3

#ifdef _WLAN_DOMAIN_
#define WLAN_DOMAIN_USA		0x10 // CH 1-11 ,REG0
#define WLAN_DOMAIN_CANADA	0x20 // CH 1-11 ,REG0
#define WLAN_DOMAIN_M_EUROPE	0x30 // CH 1-13 ,REG1
#define WLAN_DOMAIN_SPAIN	0x31 // CH 10-11,REG2     
#define WLAN_DOMAIN_FRANCE	0x32 // CH 10-13,REG3   
#define WLAN_DOMAIN_UK       	0x33 // CH 1-13 ,REG1
#define WLAN_DOMAIN_GERMAN      0x34 // CH 1-13 ,REG1
#define WLAN_DOMAIN_AUSTRALIA   0x35 // CH 1-14 ,REG5
#define WLAN_DOMAIN_JAPAN	0x40 // CH 1-13 ,REG1
#define WLAN_DOMAIN_JAPAN_ALL	0x41 // CH 1-14 ,REG5
#define WLAN_DOMAIN_S_CHINESE	0x70 // CH 1-11 ,REG0
#define WLAN_DOMAIN_T_CHINESE	0x80 // CH 1-11 ,REG0
#define WLAN_DOMAIN_KOREA	0x90 // CH 1-11 ,REG0

#define WLAN_DOMAIN_DEFAULT  WLAN_DOMAIN_USA	/* default domain */
#endif /* _WLAN_DOMAIN_ */
						/* Vincent_006: Block Stop  */
#define WLAN_MAX_BSSID_LEN	18
#define WLAN_MAX_STATUS_LEN	10
#define WLAN_USERNAME_LEN	32
#define WLAN_PASSWD_LEN		32
#define ROOT_CA			"/root/wifiserver.cer"
#define USER_CA			"/root/wifiuser.pfx"

typedef struct WLANConfig {
	int		mode;		/* Ad-Hoc, Infrastructure */
	unsigned char	essid[WLAN_MAX_ESSID_LEN+1];
        int		channel;	/* 1 ~ 14 */
	int		wep_mode;	/* disable, 64bit, 128bit */
	int		wep_authtype;	/* Open-System, Shared-Key */
        int		wep_key;	/* default key number */	/* Vincent_001 */
	unsigned char	wep_keys[WLAN_WEP_KEYS][WLAN_MAX_WEP_KEY];	/* Vincent_001: Modify */
#ifdef _WLAN_DOMAIN_
	int             domain;		/* domain area: limit channels: Vincent_006 */
#endif /* _WLAN_DOMAIN_ */
	unsigned char   passphrase[WLAN_MAX_WEP_KEY+1];  /* Generate WEP KEYs */
	int 		security;
	unsigned char   psk_pass_str[WLAN_MAX_PSK_PASS_STR+1];
	unsigned char   psk_key[WLAN_PSK_KEY_LEN];
	int		wmm;
	int		wpa_authtype;  // 0:tls 1:ttls 2:peap
	unsigned char 	wpa_tls_user[WLAN_USERNAME_LEN+1];
	unsigned char 	wpa_tls_keypwd[WLAN_PASSWD_LEN+1];
	int		wpa_ttls_authtype; //0:mschap 1:mschapv2 2:pap 3:md5
	unsigned char	wpa_ttls_user[WLAN_USERNAME_LEN+1];
	unsigned char	wpa_ttls_pwd[WLAN_PASSWD_LEN+1];
	unsigned char   wpa_ttls_anonyname[WLAN_USERNAME_LEN+1];
	int		wpa_peap_authtype; //0:mschapv2
	unsigned char	wpa_peap_user[WLAN_USERNAME_LEN+1];
	unsigned char	wpa_peap_pwd[WLAN_PASSWD_LEN+1];
	unsigned char   wpa_peap_anonyname[WLAN_USERNAME_LEN+1];
} WLANConfig;

typedef struct WLANStatus {
	char bssid[WLAN_MAX_BSSID_LEN+1];
	char ssid[WLAN_MAX_ESSID_LEN+1];
	char strength[WLAN_MAX_STATUS_LEN+1];
	int  channel;
	char speed[WLAN_MAX_STATUS_LEN+1];
} WLANStatus;

/* Read Wireless Configuration Data From The Configuration File */
int WLANReadConfigData(WLANConfig *info);
/* Write Wireless Configuration Data To The Configuration File */
int WLANWriteConfigData(WLANConfig *info);
/* Set Wireless Configuration Data To The Wireless Card */
int WLANSetConfigData(WLANConfig *info);
/*
 * Write Wireless Configuration Data To The Configuration File Then Active
 * The Change To The Wireless Card.
 */
int WLANWriteThenSetConfigData(WLANConfig *info);
/* Read Current Wireless Configuration Data From The Card (from system/device) */
int WLANReadCurrentValue(WLANConfig *info);
/* Generate the WEP KEYs from the user's input ASCII string */
int WLANGenerateWEPKeys(WLANConfig *info, char *string);
/* purpose: Read Current wireless status from the card*/
int WLANReadStatus(WLANStatus *status);
/* Check Wireless Card is TI or BROADCOM */
int WLAN_CardCheck(void);

#endif	/* _WLAN_CONFIG_H_ */
