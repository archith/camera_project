/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _LAN_CONFIG_H_
#define  _LAN_CONFIG_H_

#define  LAN_OK		 	 0
#define  LAN_ERROR		-1
#define  IP_LEN			15      /* 000.000.000.000 */
#define  DOMAIN_LEN		64

typedef struct LANConfig {
        int  bootproto;                          /* 0: fixed, 1: dhcp */
        char ip_addr[IP_LEN+1];                   
        char netmask[IP_LEN+1];                   
        char gateway[IP_LEN+1];	 		 
} LANConfig;

typedef struct DNSConfig {
	int  automatic;				 /* 0:auto ,1:User */
	char dns[IP_LEN+1];				
	char dns2[IP_LEN+1];
	char dns3[IP_LEN+1];
	char domain[DOMAIN_LEN+1];
} DNSConfig;

/*------------------------------------------------------------------*/
/* purpose: Read TCP/IP configuration data from the configuration   */	
/*	    file 						    */
/* input:   LANConfig						    */
/* return:  LAN_OK - success					    */	
/*  	    LAN_ERROR -failure					    */
/*------------------------------------------------------------------*/

int LANReadConfigData(LANConfig *info);

/*------------------------------------------------------------------*/
/* purpose: Write TCP/IP configuration data to configuration file   */
/* input:   LANConfig						    */
/* return:  LAN_OK - success					    */	
/* 	    LAN_ERROR -failure					    */
/*------------------------------------------------------------------*/

int LANWriteConfigData(LANConfig *info);

/*------------------------------------------------------------------*/
/* purpose: Set interface active				    */
/* input:   LANConfig						    */
/* return:  LAN_OK - success					    */	
/* 	    LAN_ERROR -failure					    */
/*------------------------------------------------------------------*/

int LANSetTCPIP(LANConfig *info);
int LANSetTCPIP_DHCP(LANConfig *info);

/*------------------------------------------------------------------*/
/* purpose: Set interface to default    	  		    */
/* input:   int - 0:LAN 1:wireless				    */
/* return:  LAN_OK - success					    */	
/* 	    LAN_ERROR -failure					    */
/*------------------------------------------------------------------*/

int LANSetDefaultIP(void);

/*------------------------------------------------------------------*/
/* purpose: Read DNS data from configuration file	    	    */
/* input:   DNSInfo						    */
/* return:  LAN_OK - success					    */	
/* 	    LAN_ERROR -failure					    */
/*------------------------------------------------------------------*/

int LANReadDNS(DNSConfig *info);

/*------------------------------------------------------------------*/
/* purpose: Write DNS data to configuration file	    	    */
/* input:   DNSInfo						    */
/* return:  LAN_OK - success					    */	
/* 	    LAN_ERROR -failure					    */
/*------------------------------------------------------------------*/

int LANWriteDNS(DNSConfig *info);

/*------------------------------------------------------------------*/
/* purpose: Read current interface			    	    */
/* input:   void						    */
/* return:  0: LAN 1:Wireless	 				    */	
/*------------------------------------------------------------------*/

int LANStatus(void);

/*------------------------------------------------------------------*/
/* purpose: Check wireless card				    	    */
/* input:   void						    */
/* return:  0: no wireless card 1: wireless card on board	    */	
/*------------------------------------------------------------------*/

int WCard(void);

#endif	/* _LAN_CONFIG_H_ */
