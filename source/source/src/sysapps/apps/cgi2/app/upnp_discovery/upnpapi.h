/***************************************************************
*             Copyright (C) 2003 by SerComm Corp.
*                    All Rights Reserved.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 Intel Corporation 
// All rights reserved. 
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met: 
//
// * Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer. 
// * Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution. 
// * Neither name of Intel Corporation nor the names of its contributors 
// may be used to endorse or promote products derived from this software 
// without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////
//
// $Revision: 1.1.1.1 $
// $Date: 2006/03/24 07:00:48 $
//
// Change Log: 
//   extract from inc/upnp.h, inc/interface.h

#ifndef _UPNPAPI_H_
#define _UPNPAPI_H_

#include <ssdpparser.h>
/* export from inc/upnp.h */
typedef int UpnpDevice_Handle;

#define DEFAULT_MAXAGE 1800

/* export from inc/interface.h */
#define SERVER	"LINUX"


//typedef enum CmdType{ERROR=-1,OK,ALIVE,BYEBYE,SEARCH,NOTIFY,TIMEOUT} Cmd;
//typedef enum CmdType{ERROR=-1,OK,ALIVE,BYEBYE,SEARCH,NOTIFY,TIMEOUT} Cmd;
//typedef enum SEARCH_TYPE{SERROR=-1,ALL,ROOTDEVICE,DEVICE,DEVICETYPE,SERVICE} SearchType;
//typedef enum SsdpCmdType{SSDP_ERROR=-1,SSDP_OK,SSDP_ALIVE,SSDP_BYEBYE,
                         //SSDP_SEARCH,SSDP_NOTIFY,SSDP_TIMEOUT} CmdT;
//typedef enum SsdpSearchType{SSDP_SERROR=-1,SSDP_ALL,SSDP_ROOTDEVICE,
                           //SSDP_DEVICE,SSDP_DEVICETYPE,SSDP_SERVICE} SType;


/* copy from upnp.h */
#define LINE_SIZE	280

//typedef struct SsdpEventStruct
//{
    //enum CmdType Cmd;
    //enum SEARCH_TYPE RequestType;
    //int  ErrCode;
    //int  MaxAge;
    //int  Mx;
    //char UDN[LINE_SIZE];
    //char DeviceType[LINE_SIZE];
    //char ServiceType[LINE_SIZE];  //NT or ST
    //char Location[LINE_SIZE];
    //char HostAddr[LINE_SIZE];
    //char Os[LINE_SIZE];
    //char Ext[LINE_SIZE];
    //char Date[LINE_SIZE];
    //char Man[LINE_SIZE];
    //struct sockaddr_in * DestAddr;
    //void * Cookie;
//} Event;

//typedef void (* SsdpFunPtr)(Event *);
//typedef Event SsdpEvent ;



#define EXCLUDE_SSDP		0
#define EXCLUDE_MINISERVER	0
#define EXCLUDE_SOAP 		1
#define EXCLUDE_GENA 		1

/* UPNP API FUNCTION */
//int UpnpInit(const char *HostIP, unsigned short DestPort);
int UpnpInit(unsigned short DestPort);
int UpnpFinish(void);

int NewUpnpRegisterRootDevice (const char *DescUrl, UpnpDevice_Handle *Hnd);
//int NewUpnpUnRegisterRootDevice(UpnpDevice_Handle Hnd);
int NewUpnpSendAdvertisement(UpnpDevice_Handle Hnd, int Exp);
int UpnpRemoveRootDevice(UpnpDevice_Handle Hnd);
int AdvertiseAndReply(int AdFlag, UpnpDevice_Handle Hnd, 
                  enum SsdpSearchType SearchType, struct sockaddr_in *DestAddr,
                      char *DeviceType, char *DeviceUDN, 
                      char *ServiceType, int Exp);



#endif
