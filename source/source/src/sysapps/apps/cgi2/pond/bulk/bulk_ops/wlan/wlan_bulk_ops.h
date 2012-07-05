/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _WIRELESS_BILK_OPS_H_
#define  _WIRELESS_BILK_OPS_H_
#include <WLAN_config.h>

#define WLAN_BULK_NAME		"WIRELESS"
#define WLAN_ACTION_REBOOT	0
#define WLAN_ACTION_NOW		1
#define WLAN_ACTION_ONCHANGE	2
struct WLAN_DS
{
	int  type; 	/* 0:Ad-Hoc  1:Infrastructure */ 
	char essid[WLAN_MAX_ESSID_LEN+1];
	int  channel;	/* 1 ~ 14 */
	int  domain;
	//  0:USA       (1-11)   1:Canada     (1-11)   2:Most of Europe (1-13)
	//  3:Spain     (10-11)  4:France     (10-13)  5:UK 		(1-13)
	//  6:German    (1-13)   7:Austrialia (1-14)   8:Japan 		(1-13) 
	//  9:Japan All (1-14)  10:S Chinese  (1-11)   11:T Chinese 	(1-11) 
	// 12:Korea     (1-11)
	int  security;  /*  */
	int  wep_authtype;		 
	int  wep_mode;		 
	int  wep_index;		 
	char wep_key[WLAN_WEP_KEYS][WLAN_MAX_WEP_KEY];		 
	char wpa_ascii[WLAN_MAX_PSK_PASS_STR+1];
	int  wmm;
	int  WpaAuthType;  /* 0:tls 1:ttls */
	char Wpa_TLSUser[WLAN_USERNAME_LEN+1];
	char Wpa_TLSPrivateKeyPasswd[WLAN_PASSWD_LEN+1];
	int  Wpa_TTLSAuthType; /* 0:MSCHAP 1:MSCHAPv2 2.PEAP 3:MD5 */
	char Wpa_TTLSUser[WLAN_USERNAME_LEN+1];
	char Wpa_TTLSPasswd[WLAN_PASSWD_LEN+1];
	char Wpa_TTLSAnonyname[WLAN_USERNAME_LEN+1];
	int  Wpa_PEAPAuthType; /* 0:MSCHAPV2 */
	char Wpa_PEAPUser[WLAN_USERNAME_LEN+1];
	char Wpa_PEAPPasswd[WLAN_PASSWD_LEN+1];
	char Wpa_PEAPAnonyname[WLAN_USERNAME_LEN+1];
	char wep_ascii[WLAN_MAX_WEP_KEY+1];
	int  action;	       /* 0:OnReboot 1:Now 2:OnChange  */
	int  card;	       /* 0: no wireless card 1: wireless card on board */
	WLANStatus wlan_info;
};

/* Generate the WEP KEYs from the user's input ASCII string */
int WLAN_Generate_WEPKeys(void *ds);
int WLAN_BULK_ReadDS(void* ds);
int WLAN_BULK_CheckDS(void* ds, void* ds_org);
int WLAN_BULK_WriteDS(void* ds, void* ds_org);
int WLAN_BULK_RunDS(void* ds, void* ds_org);
int WLAN_BULK_WebMsg(int errcode, char* message, int* type);

#endif

