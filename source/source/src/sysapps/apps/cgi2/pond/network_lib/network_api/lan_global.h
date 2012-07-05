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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <profile.h>
#include "model.h"
#define DNS_CONF	"/etc/resolv.conf"
#define BOOTCONN_CONF	"/tmp/connect_status"
#define WCARD_CONF	"/tmp/wcard"
#define LANCARD		0
#define WCARD		1
#define DHCPCD_PID 	"/var/run/dhcpcd-eth0.pid"
#define DHCPCD_INFO     "/tmp/dhcpc/dhcpcd-eth0.info"
#define	dhcp		1
#define fixed		0
#define WAIT_DHCPCD_TIME 3
#define WAIT_CONNECT_TIME 2
#define STR_NULL_IP	"0.0.0.0"

#if (OEM == OEM_Linksys)
#define DEF_IPADDR	"192.168.1.115"
#else
#define DEF_IPADDR	"192.168.0.99"
#endif
#define DEF_NETMASK	"255.255.255.0"
#define DEF_SUBNET	"192.168.0.0"

#define LAN		0
#define WLAN		1
#define NO_LINK   	0x40

#define DEF_DNS		"168.95.192.1"
