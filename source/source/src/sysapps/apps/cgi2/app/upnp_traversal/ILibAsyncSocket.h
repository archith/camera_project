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
 * $Workfile: ILibAsyncSocket.h
 * $Revision: #1.0.1799.42459
 * $Author:   Intel Corporation, Intel Device Builder
 * $Date:     2007¦~5¤ë31¤é
 *
 *
 *
 */

#ifndef ___ILibAsyncSocket___
#define ___ILibAsyncSocket___

#if defined(WIN32) || defined(_WIN32_WCE)
#include <STDDEF.H>
#else
#include <malloc.h>
#endif

#define ILibAsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR 2

enum ILibAsyncSocket_MemoryOwnership
{
	ILibAsyncSocket_MemoryOwnership_CHAIN=0,
	ILibAsyncSocket_MemoryOwnership_STATIC=1,
	ILibAsyncSocket_MemoryOwnership_USER=2
};

typedef void(**ILibAsyncSocket_OnInterrupt)(void *socketModule, void *user);
typedef void(*ILibAsyncSocket_OnData)(void* socketModule,char* buffer,int *p_beginPointer, int endPointer,ILibAsyncSocket_OnInterrupt OnInterrupt, void **user, int *PAUSE);
typedef void(*ILibAsyncSocket_OnConnect)(void* socketModule, int Connected, void *user);
typedef void(*ILibAsyncSocket_OnDisconnect)(void* socketModule, void *user);
typedef void(*ILibAsyncSocket_OnSendOK)(void *socketModule, void *user);
typedef void (*ILibAsyncSocket_OnBufferReAllocated)(void *AsyncSocketToken, void *user, ptrdiff_t newOffset);
void ILibAsyncSocket_SetReAllocateNotificationCallback(void *AsyncSocketToken, ILibAsyncSocket_OnBufferReAllocated Callback);
void * ILibAsyncSocket_GetUser(void *socketModule);

void* ILibCreateAsyncSocketModule(void *Chain, int initialBufferSize, void(*OnData)(void* socketModule,char* buffer,int *p_beginPointer, int endPointer,void (**InterruptPtr)(void *socketModule, void *user), void **user, int *PAUSE), void(*OnConnect)(void* socketModule, int Connected, void *user),void(*OnDisconnect)(void* socketModule, void *user),void(*OnSendOK)(void *socketModule, void *user));
unsigned int ILibAsyncSocket_GetPendingBytesToSend(void *socketModule);
unsigned int ILibAsyncSocket_GetTotalBytesSent(void *socketModule);
void ILibAsyncSocket_ResetTotalBytesSent(void *socketModule);

void ILibAsyncSocket_ConnectTo(void* socketModule, int localInterface, int remoteInterface, int remotePortNumber,void (*InterruptPtr)(void *socketModule, void *user),void *user);
int ILibAsyncSocket_Send(void* socketModule, char* buffer, int length, enum ILibAsyncSocket_MemoryOwnership UserFree);
void ILibAsyncSocket_Disconnect(void* socketModule);
void ILibAsyncSocket_GetBuffer(void *socketModule, char **buffer, int *BeginPointer, int *EndPointer);

void ILibAsyncSocket_UseThisSocket(void *socketModule,void* TheSocket,void (*InterruptPtr)(void *socketModule, void *user),void *user);
void ILibAsyncSocket_SetRemoteAddress(void *socketModule,int RemoteAddress);

int ILibAsyncSocket_IsFree(void *socketModule);
int ILibAsyncSocket_GetLocalInterface(void *socketModule);
int ILibAsyncSocket_GetRemoteInterface(void *socketModule);


char* ILibGetReceivingInterface(void* ReaderObject);
void ILibAsyncSocket_Resume(void *socketModule);
#endif
