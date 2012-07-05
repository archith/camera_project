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
//   currentTmToHttpDate() : extract from src/genlib/http_client/http_client.c
//   other function : extract from ssdp/ssdplib.c

#ifndef SSDPLIB_H
#define SSDPLIB_H 

#define MULTICAST_LISTENER_NAME		"ListenMcast"
//#define MULTICAST_LISTENER		"/root/ListenMcast"
#define MULTICAST_LISTENER		"/usr/local/bin/ListenMcast"


//Constant
#define	 BUFSIZE   2500
#define  SSDP_IP   "239.255.255.250"
#define  SSDP_PORT 1900
#define  NUM_TRY 3
#define  NUM_COPY 2
#define  THREAD_LIMIT 50
#define  COMMAND_LEN  300

//Error code
#define NO_ERROR_FOUND    0
#define E_REQUEST_INVALID  	-3
#define E_RES_EXPIRED		-4
#define E_MEM_ALLOC		-5
#define E_HTTP_SYNTEX		-6
#define E_SOCKET 		-7
#define RQST_TIMEOUT    20


/* globals */
//extern int errno;
//struct timeval trt;


/* export function */
int InitSsdpLib(void);

int DeviceAdvertisement(char * DevType, int RootDev,char * Udn, char *Server, char * Location, int  Duration);

int DeviceShutdown(char * DevType, int RootDev,char * Udn, char *Server, char * Location, int  Duration);

int DeviceReply(struct sockaddr_in * DestAddr, char *DevType, int RootDev, char * Udn, char *Server, char * Location, int  Duration);

int SendReply(struct sockaddr_in * DestAddr, char *DevType, int RootDev, char * Udn, char *Server, char * Location, int  Duration, int ByType);

int ServiceAdvertisement( char * Udn, char * ServType,char *Server,char * Location,int  Duration);


void DeInitSsdpLib(void);


#endif
