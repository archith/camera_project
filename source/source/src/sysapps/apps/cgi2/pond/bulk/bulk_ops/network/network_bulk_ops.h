/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _NETWORK_BILK_OPS_H_
#define  _NETWORK_BILK_OPS_H_

#include <lan_config.h>
#define NET_BULK_NAME		"NETWORK"

#define	 ADDR_LEN		16

struct NET_DS
{
	int  network_type;	 // 0 : LAN 1:WLAN
	char ip_addr[ADDR_LEN+1];
 	char netmask[ADDR_LEN+1];
	char gateway[ADDR_LEN+1];
	//int  dhcp;		 // 0 : dhcp client 1: static
	int  bootproto;		 // 1: dhcp client 0: static
	int  dns_type;		 
	char dns_server1[ADDR_LEN+1];
	char dns_server2[ADDR_LEN+1];
	char domain[DOMAIN_LEN+1];
	int  wcard;		// 0  : no card  1:wireless card on board
};

int NET_BULK_ReadDS(void* ds);
int NET_BULK_CheckDS(void* ds, void* ds_org);
int NET_BULK_WriteDS(void* ds, void* ds_org);
int NET_BULK_RunDS(void* ds, void* ds_org);
int NET_BULK_WebMsg(int errcode, char* message, int* type);
#endif

