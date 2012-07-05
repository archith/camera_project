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
//   extract from inc/upnp.h

#ifndef UPNP_H
#define UPNP_H

#ifdef DEBUG
#define DBGONLY(x) x	
#define DBG(x) x
#else
#define DBGONLY(x)
#define DBG(x)
#endif
/* export from inc/upnp.h */
#define LINE_SIZE	280
#define UPNP_E_SUCCESS		0

#define UPNP_E_INVALID_HANDLE   -100
#define UPNP_E_OUTOF_MEMORY	-104
#define UPNP_E_INIT             -105

#define UPNP_E_FINISH           -116
#define UPNP_E_INIT_FAILED      -117

#define UPNP_E_SOCKET_BIND      -203
#define UPNP_E_OUTOF_SOCKET	-205
#define UPNP_E_LISTEN           -206

#define UPNP_E_INTERNAL_ERROR         -911


/* sam add */
#define UPNP_E_RUN_MINISRV	-301


#endif
