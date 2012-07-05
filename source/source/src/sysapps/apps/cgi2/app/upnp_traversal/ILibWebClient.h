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
 * $Workfile: ILibWebClient.h
 * $Revision: #1.0.1799.42459
 * $Author:   Intel Corporation, Intel Device Builder
 * $Date:     2007¦~5¤ë31¤é
 *
 *
 *
 */

#ifndef __ILibWebClient__
#define __ILibWebClient__

#define WEBCLIENT_DESTROYED 5
#define WEBCLIENT_DELETED 6

#if defined(WIN32) || defined(_WIN32_WCE)
#include <STDDEF.h>
#else
#include <malloc.h>
#endif

void *ILibCreateWebClient(int PoolSize,void *Chain);
void *ILibCreateWebClientEx(void (*OnResponse)(
						void *WebReaderToken,
						int InterruptFlag,
						struct packetheader *header,
						char *bodyBuffer,
						int *beginPointer,
						int endPointer,
						int done,
						void *user1,
						void *user2,
						int *PAUSE), void *socketModule, void *user1, void *user2);

void ILibWebClient_OnBufferReAllocate(void *token, void *user, ptrdiff_t offSet);
void ILibWebClient_OnData(void* socketModule,char* buffer,int *p_beginPointer, int endPointer,void (**InterruptPtr)(void *socketModule, void *user), void **user, int *PAUSE);
void ILibDestroyWebClient(void *object);

void ILibWebClient_DestroyWebClientDataObject(void *token);
struct packetheader *ILibWebClient_GetHeaderFromDataObject(void *token);

void ILibWebClient_PipelineRequestEx(
	void *WebClient, 
	struct sockaddr_in *RemoteEndpoint, 
	char *headerBuffer,
	int headerBufferLength,
	int headerBuffer_FREE,
	char *bodyBuffer,
	int bodyBufferLength,
	int bodyBuffer_FREE,
	void (*OnResponse)(
		void *WebReaderToken,
		int InterruptFlag,
		struct packetheader *header,
		char *bodyBuffer,
		int *beginPointer,
		int endPointer,
		int done,
		void *user1,
		void *user2,
		int *PAUSE),
	void *user1,
	void *user2);
void ILibWebClient_PipelineRequest(
	void *WebClient, 
	struct sockaddr_in *RemoteEndpoint, 
	struct packetheader *packet,
	void (*OnResponse)(
		void *WebReaderToken,
		int InterruptFlag,
		struct packetheader *header,
		char *bodyBuffer,
		int *beginPointer,
		int endPointer,
		int done,
		void *user1,
		void *user2,
		int *PAUSE),
	void *user1,
	void *user2);

void ILibWebClient_FinishedResponse_Server(void *wcdo);
void ILibWebClient_DeleteRequests(void *WebClientToken,char *IP,int Port);
void ILibWebClient_Resume(void *wcdo);
void ILibWebClient_Disconnect(void *wcdo);

#endif
