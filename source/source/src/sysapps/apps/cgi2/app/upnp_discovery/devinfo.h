/***************************************************************
*             Copyright (C) 2003 by SerComm Corp.
*                    All Rights Reserved.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#define UPNP_PORT	6789
#define UPNP_XML_FN	"device.xml"
#define UPNP_XML_PATH	"/tmp/device.xml"
#define UPNP_XML_BASIC_SERVICE_PATH		"/tmp/scpd_basic.xml"
#define UPNP_XML_BASIC_SERVICE_FN		"/scpd_basic.xml"


#define UPNP_MAXLEN	256


typedef struct Upnp_Data{
	char device_name[UPNP_MAXLEN+1];		//only name
	char friendly_name[UPNP_MAXLEN+1];	//<friendlyName>
	char manufacturer[UPNP_MAXLEN+1]; 	//<manufacturer>
	char manufacturerURL[UPNP_MAXLEN+1];	//<manufacturerURL>
	char model_description[UPNP_MAXLEN+1];	//<modelDescription>
	char model_name[UPNP_MAXLEN+1];		//<modelName>
	char model_number[UPNP_MAXLEN+1];		//<modelNumber>
}Upnp_Data;

typedef struct Upnp_Data1{
	char uuid[UPNP_MAXLEN];			//<UDN>
	char location[UPNP_MAXLEN];		//<URLBase>			
	char device_type[UPNP_MAXLEN];		//<deviceType>
}Upnp_Data1;

int SetUPnPDeviceInfo(void);

#define DEVICE_OK			0
#define DEVICE_READ_LAN_ERR		-1
#define DEVICE_GEN_XML_ERR		-2

#endif
