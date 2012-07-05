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
 * $Workfile: UPnPControlPoint.h
 * $Revision: #1.0.1799.42459
 * $Author:   Intel Corporation, Intel Device Builder
 * $Date:     2007¦~5¤ë31¤é
 *
 *
 *
 */
#ifndef __UPnPControlPoint__
#define __UPnPControlPoint__

#include "UPnPControlPointStructs.h"




/* Complex Type Parsers */


/* Complex Type Serializers */



void UPnPAddRef(struct UPnPDevice *device);
void UPnPRelease(struct UPnPDevice *device);

struct UPnPDevice* UPnPGetDevice1(struct UPnPDevice *device,int index);
int UPnPGetDeviceCount(struct UPnPDevice *device);
struct UPnPDevice* UPnPGetDeviceAtUDN(void *v_CP,char* UDN);

void PrintUPnPDevice(int indents, struct UPnPDevice *device);


void UPnPStopCP(void *v_CP);
void *UPnPCreateControlPoint(void *Chain, void(*A)(struct UPnPDevice*),void(*R)(struct UPnPDevice*));
void UPnP_CP_IPAddressListChanged(void *CPToken);
struct UPnPDevice* UPnPGetDeviceEx(struct UPnPDevice *device, char* DeviceType, int start,int number);
int UPnPHasAction(struct UPnPService *s, char* action);
void UPnPUnSubscribeUPnPEvents(struct UPnPService *service);
void UPnPSubscribeForUPnPEvents(struct UPnPService *service, void(*callbackPtr)(struct UPnPService* service,int OK));
struct UPnPService *UPnPGetService(struct UPnPDevice *device, char* ServiceName, int length);

struct UPnPService *UPnPGetService_WANIPConnection(struct UPnPDevice *device);


extern void (*UPnPEventCallback_WANIPConnection_PortMappingNumberOfEntries)(struct UPnPService* Service,unsigned short PortMappingNumberOfEntries);
extern void (*UPnPEventCallback_WANIPConnection_ConnectionStatus)(struct UPnPService* Service,char* ConnectionStatus);
extern void (*UPnPEventCallback_WANIPConnection_ExternalIPAddress)(struct UPnPService* Service,char* ExternalIPAddress);
extern void (*UPnPEventCallback_WANIPConnection_PossibleConnectionTypes)(struct UPnPService* Service,char* PossibleConnectionTypes);


void UPnPInvoke_WANIPConnection_AddPortMapping(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService*,int ErrorCode,void *user),void* _user, char* NewRemoteHost, unsigned short NewExternalPort, char* NewProtocol, unsigned short NewInternalPort, char* NewInternalClient, int NewEnabled, char* NewPortMappingDescription, unsigned int NewLeaseDuration);
void UPnPInvoke_WANIPConnection_DeletePortMapping(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService*,int ErrorCode,void *user),void* _user, char* NewRemoteHost, unsigned short NewExternalPort, char* NewProtocol);
void UPnPInvoke_WANIPConnection_GetExternalIPAddress(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService*,int ErrorCode,void *user,char* NewExternalIPAddress),void* _user);
void UPnPInvoke_WANIPConnection_GetSpecificPortMappingEntry(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService*,int ErrorCode,void *user,unsigned short NewInternalPort,char* NewInternalClient,int NewEnabled,char* NewPortMappingDescription,unsigned int NewLeaseDuration),void* _user, char* NewRemoteHost, unsigned short NewExternalPort, char* NewProtocol);


#endif
