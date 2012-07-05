/*
 * INTEL CONFIDENTIAL
 * Copyright (c) 2002, 2003 Intel Corporation.  All rights reserved.
 * 
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel
 * Corporation or its suppliers or licensors.  Title to the
 * Material remains with Intel Corporation or its suppliers and
 * licensors.  The Material contains trade secrets and proprietary
 * and confidential information of Intel or its suppliers and
 * licensors. The Material is protected by worldwide copyright and
 * trade secret laws and treaty provisions.  No part of the Material
 * may be used, copied, reproduced, modified, published, uploaded,
 * posted, transmitted, distributed, or disclosed in any way without
 * Intel's prior express written permission.
 
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you
 * by disclosure or delivery of the Materials, either expressly, by
 * implication, inducement, estoppel or otherwise. Any license
 * under such intellectual property rights must be express and
 * approved by Intel in writing.
 * 
 * $Workfile: ILibWebServer.h
 * $Revision: #1.0.1799.42459
 * $Author:   Intel Corporation, Intel Device Builder
 * $Date:     2007¦~5¤ë31¤é
 *
 *
 *
 */

#ifndef __ILibWebServer__
#define __ILibWebServer__

#define ILibWebServer_SEND_RESULTED_IN_DISCONNECT -2
#define ILibWebServer_INVALID_SESSION -3

struct ILibWebServer_Session;
typedef void (*ILibWebServer_Session_OnReceive)\
				(struct ILibWebServer_Session *sender,\
					int InterruptFlag,\
					struct packetheader *header,\
					char *bodyBuffer,\
					int *beginPointer,\
					int endPointer,\
					int done);

struct ILibWebServer_Session
{
	ILibWebServer_Session_OnReceive OnReceive;
	void (*OnDisconnect)(struct ILibWebServer_Session *sender);
	void (*OnSendOK)(struct ILibWebServer_Session *sender);
	void *Parent;
	void *User;
	void *User2;

	void *Reserved1;	// AsyncServerSocket
	void *Reserved2;	// ConnectionToken
	void *Reserved3;	// WebClientDataObject
	void *Reserved7;	// VirtualDirectory
	int Reserved4;	// Request Answered Flag (set by send)
	int Reserved8;	// RequestAnswered Method Called
	int Reserved5;	// Request Made Flag
	int Reserved6;	// Close Override Flag
	int Reserved9;	// Reserved for future use
	void ** Reserved10;	// DisconnectFlagPointer
};

typedef void (*ILibWebServer_Session_OnSession)(struct ILibWebServer_Session *SessionToken, void *User);
typedef void (*ILibWebServer_VirtualDirectory)(struct ILibWebServer_Session *session, struct packetheader *header, char *bodyBuffer, int *beginPointer, int endPointer, int done, void *user);

void ILibWebServer_SetTag(void *WebServerToken, void *Tag);
void *ILibWebServer_GetTag(void *WebServerToken);

void *ILibWebServer_Create(void *Chain, int MaxConnections, int PortNumber,ILibWebServer_Session_OnSession OnSession, void *User);
int ILibWebServer_RegisterVirtualDirectory(void *WebServerToken, char *vd, int vdLength, ILibWebServer_VirtualDirectory OnVirtualDirectory, void *user);
int ILibWebServer_UnRegisterVirtualDirectory(void *WebServerToken, char *vd, int vdLength);

int ILibWebServer_Send(struct ILibWebServer_Session *session, struct packetheader *packet);
int ILibWebServer_Send_Raw(struct ILibWebServer_Session *session, char *buffer, int bufferSize, int userFree, int done);

#define ILibWebServer_Session_GetPendingBytesToSend(session) ILibAsyncServerSocket_GetPendingBytesToSend(session->Reserved1,session->Reserved2)
#define ILibWebServer_Session_GetTotalBytesSent(session) ILibAsyncServerSocket_GetTotalBytesSent(session->Reserved1,session->Reserved2)
#define ILibWebServer_Session_ResetTotalBytesSent(session) ILibAsyncServerSocket_ResetTotalBytesSent(session->Reserved1,session->Reserved2)

unsigned short ILibWebServer_GetPortNumber(void *WebServerToken);
int ILibWebServer_GetLocalInterface(struct ILibWebServer_Session *session);
int ILibWebServer_GetRemoteInterface(struct ILibWebServer_Session *session);

int ILibWebServer_StreamHeader(struct ILibWebServer_Session *session, struct packetheader *header);
int ILibWebServer_StreamBody(struct ILibWebServer_Session *session, char *buffer, int bufferSize, int userFree, int done);

int ILibWebServer_StreamHeader_Raw(struct ILibWebServer_Session *session, int StatusCode,char *StatusData,char *ResponseHeaders, int ResponseHeaders_FREE);

#endif
