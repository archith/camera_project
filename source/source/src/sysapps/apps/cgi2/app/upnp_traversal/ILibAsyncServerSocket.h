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
 * $Workfile: ILibAsyncServerSocket.h
 * $Revision: #1.0.1799.42459
 * $Author:   Intel Corporation, Intel Device Builder
 * $Date:     2007¦~5¤ë31¤é
 *
 *
 *
 */

#ifndef ___ILibAsyncServerSocket___
#define ___ILibAsyncServerSocket___

#if defined(WIN32) || defined(_WIN32_WCE)
#include <STDDEF.H>
#else
#include <malloc.h>
#endif

typedef void (*ILibAsyncServerSocket_BufferReAllocated)(void *AsyncServerSocketToken, void *ConnectionToken, void *user, ptrdiff_t newOffset);
void ILibAsyncServerSocket_SetReAllocateNotificationCallback(void *AsyncServerSocketToken, void *ConnectionToken, ILibAsyncServerSocket_BufferReAllocated Callback);


void *ILibCreateAsyncServerSocketModule(void *Chain, int MaxConnections, int PortNumber, int initialBufferSize, void (*OnConnect)(void *AsyncServerSocketModule, void *ConnectionToken,void **user),void (*OnDisconnect)(void *AsyncServerSocketModule, void *ConnectionToken, void *user),void (*OnReceive)(void *AsyncServerSocketModule, void *ConnectionToken,char* buffer,int *p_beginPointer, int endPointer,void (**OnInterrupt)(void *AsyncServerSocketMoudle, void *ConnectionToken, void *user), void **user, int *PAUSE),void (*OnInterrupt)(void *AsyncServerSocketModule, void *ConnectionToken, void *user),void (*OnSendOK)(void *AsyncServerSocketModule,void *ConnectionToken, void *user));

void *ILibAsyncServerSocket_GetTag(void *AsyncSocketModule);
void ILibAsyncServerSocket_SetTag(void *AsyncSocketModule, void *user);

unsigned short ILibAsyncServerSocket_GetPortNumber(void *ServerSocketModule);

#define ILibAsyncServerSocket_Send(ServerSocketModule, ConnectionToken, buffer, bufferLength, UserFreeBuffer) ILibAsyncSocket_Send(ConnectionToken,buffer,bufferLength,UserFreeBuffer)
#define ILibAsyncServerSocket_Disconnect(ServerSocketModule, ConnectionToken) ILibAsyncSocket_Disconnect(ConnectionToken)
#define ILibAsyncServerSocket_GetPendingBytesToSend(ServerSocketModule, ConnectionToken) ILibAsyncSocket_GetPendingBytesToSend(ConnectionToken)
#define ILibAsyncServerSocket_GetTotalBytesSent(ServerSocketModule, ConnectionToken) ILibAsyncSocket_GetTotalBytesSent(ConnectionToken)
#define ILibAsyncServerSocket_ResetTotalBytesSent(ServerSocketModule, ConnectionToken) ILibAsyncSocket_ResetTotalBytesSent(ConnectionToken)

#endif
