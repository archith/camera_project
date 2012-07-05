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
 * $Workfile: ILibParsers.h
 * $Revision: #1.0.1799.42459
 * $Author:   Intel Corporation, Intel Device Builder
 * $Date:     2007¦~5¤ë31¤é
 *
 *
 *
 */

#ifndef __ILibParsers__
#define __ILibParsers__

#define MAX_HEADER_LENGTH 800
#if defined(WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#	ifdef WINSOCK1
#	include <winsock.h>
#	endif
	#define sem_t HANDLE
	#define sem_init(x,y,z) *x=CreateSemaphore(NULL,z,FD_SETSIZE,NULL)
	#define sem_destroy(x) (CloseHandle(*x)==0?1:0)
	#define sem_wait(x) WaitForSingleObject(*x,INFINITE)
	#define sem_trywait(x) ((WaitForSingleObject(*x,0)==WAIT_OBJECT_0)?0:1)
	#define sem_post(x) ReleaseSemaphore(*x,1,NULL)

	#define strncasecmp(x,y,z) _strnicmp(x,y,z)
	#define strcasecmp(x,y) _stricmp(x,y)
	#define gettimeofday(x,y) (x)->tv_sec = GetTickCount()/1000;(x)->tv_usec = 1000*(GetTickCount()%1000)
#endif


#define UPnPMIN(a,b) (((a)<(b))?(a):(b))
#define ILibIsChainBeingDestroyed(Chain) (*((int*)Chain))

typedef enum
{
	ILibServerScope_All=0,
	ILibServerScope_LocalLoopback=1,
	ILibServerScope_LocalSegment=2
}ILibServerScope;
struct parser_result_field
{
	char* data;
	int datalength;
	struct parser_result_field *NextResult;
};
struct parser_result
{
	struct parser_result_field *FirstResult;
	struct parser_result_field *LastResult;
	int NumResults;
};
struct packetheader_field_node
{
	char* Field;
	int FieldLength;
	char* FieldData;
	int FieldDataLength;
	int UserAllocStrings;
	struct packetheader_field_node* NextField;
};
struct packetheader
{
	char* Directive;
	int DirectiveLength;
	char* DirectiveObj;
	int DirectiveObjLength;
	int StatusCode;
	char* StatusData;
	int StatusDataLength;
	char* Version;
	int VersionLength;
	char* Body;
	int BodyLength;
	int UserAllocStrings;
	int UserAllocVersion;
	int ClonedPacket;
	
	struct packetheader_field_node* FirstField;
	struct packetheader_field_node* LastField;
	struct sockaddr_in *Source;
	int ReceivingAddress;
};
struct ILibXMLNode
{
	char* Name;
	int NameLength;
	
	char* NSTag;
	int NSLength;
	int StartTag;
	int EmptyTag;
	
	void *Reserved;
	void *Reserved2;
	struct ILibXMLNode *Next;
	struct ILibXMLNode *Parent;
	struct ILibXMLNode *Peer;
	struct ILibXMLNode *ClosingTag;
	struct ILibXMLNode *StartingTag;
};
struct ILibXMLAttribute
{
	char* Name;
	int NameLength;
	
	char* Prefix;
	int PrefixLength;
	
	struct ILibXMLNode *Parent;

	char* Value;
	int ValueLength;
	struct ILibXMLAttribute *Next;
};

int ILibFindEntryInTable(char *Entry, char **Table);

char *ILibReadFileFromDisk(char *FileName);
int ILibReadFileFromDiskEx(char **Target, char *FileName);
void ILibWriteStringToDisk(char *FileName, char *data);

/* Stack Methods */
void ILibCreateStack(void **TheStack);
void ILibPushStack(void **TheStack, void *data);
void *ILibPopStack(void **TheStack);
void *ILibPeekStack(void **TheStack);
void ILibClearStack(void **TheStack);

/* Queue Methods */
void *ILibQueue_Create();
void ILibQueue_Destroy(void *q);
int ILibQueue_IsEmpty(void *q);
void ILibQueue_EnQueue(void *q, void *data);
void *ILibQueue_DeQueue(void *q);
void *ILibQueue_PeekQueue(void *q);
void ILibQueue_Lock(void *q);
void ILibQueue_UnLock(void *q);

/* XML Parsing Methods */
void ILibXML_BuildNamespaceLookupTable(struct ILibXMLNode *node);
char* ILibXML_LookupNamespace(struct ILibXMLNode *currentLocation, char *prefix, int prefixLength);
int ILibReadInnerXML(struct ILibXMLNode *node, char **RetVal);
struct ILibXMLNode *ILibParseXML(char *buffer, int offset, int length);
struct ILibXMLAttribute *ILibGetXMLAttributes(struct ILibXMLNode *node);
int ILibProcessXMLNodeList(struct ILibXMLNode *nodeList);
void ILibDestructXMLNodeList(struct ILibXMLNode *node);
void ILibDestructXMLAttributeList(struct ILibXMLAttribute *attribute);

/* Chaining Methods */
void *ILibCreateChain();
void ILibAddToChain(void *chain, void *object);
void ILibStartChain(void *chain);
void ILibStopChain(void *chain);
void ILibForceUnBlockChain(void *Chain);

/* Linked List Methods */
void* ILibLinkedList_Create();
void* ILibLinkedList_GetNode_Head(void *LinkedList); // Returns Node
void* ILibLinkedList_GetNode_Tail(void *LinkedList); // Returns Node
void* ILibLinkedList_GetNextNode(void *LinkedList_Node); // Returns Node
void* ILibLinkedList_GetPreviousNode(void *LinkedList_Node); // Returns Node
long ILibLinkedList_GetCount(void *LinkedList);
void* ILibLinkedList_ShallowCopy(void *LinkedList);

void *ILibLinkedList_GetDataFromNode(void *LinkedList_Node);
void ILibLinkedList_InsertBefore(void *LinkedList_Node, void *data);
void ILibLinkedList_InsertAfter(void *LinkedList_Node, void *data);
void ILibLinkedList_Remove(void *LinkedList_Node);
void ILibLinkedList_Remove_ByData(void *LinkedList, void *data);
void ILibLinkedList_AddHead(void *LinkedList, void *data);
void ILibLinkedList_AddTail(void *LinkedList, void *data);

void ILibLinkedList_Lock(void *LinkedList);
void ILibLinkedList_UnLock(void *LinkedList);
void ILibLinkedList_Destroy(void *LinkedList);



/* HashTree Methods */
void* ILibInitHashTree();
void ILibDestroyHashTree(void *tree);
int ILibHasEntry(void *hashtree, char* key, int keylength);
void ILibAddEntry(void* hashtree, char* key, int keylength, void *value);
void* ILibGetEntry(void *hashtree, char* key, int keylength);
void ILibDeleteEntry(void *hashtree, char* key, int keylength);
void *ILibHashTree_GetEnumerator(void *tree);
void ILibHashTree_DestroyEnumerator(void *tree_enumerator);
int ILibHashTree_MoveNext(void *tree_enumerator);
void ILibHashTree_GetValue(void *tree_enumerator, char **key, int *keyLength, void **data);
void ILibHashTree_Lock(void *hashtree);
void ILibHashTree_UnLock(void *hashtree);

/* LifeTimeMonitor Methods */
#define ILibLifeTime_Add(LifetimeMonitorObject, data, seconds, Callback, Destroy) ILibLifeTime_AddEx(LifetimeMonitorObject, data, seconds*1000, Callback, Destroy)
void ILibLifeTime_AddEx(void *LifetimeMonitorObject,void *data, int milliseconds, void* Callback, void* Destroy);
void ILibLifeTime_Remove(void *LifeTimeToken, void *data);
void ILibLifeTime_Flush(void *LifeTimeToken);
void *ILibCreateLifeTime(void *Chain);

/* String Parsing Methods */
int ILibTrimString(char **theString, int length);
struct parser_result* ILibParseString(char* buffer, int offset, int length, char* Delimiter, int DelimiterLength);
struct parser_result* ILibParseStringAdv(char* buffer, int offset, int length, char* Delimiter, int DelimiterLength);
void ILibDestructParserResults(struct parser_result *result);
void ILibParseUri(char* URI, char** IP, unsigned short* Port, char** Path);
int ILibGetLong(char *TestValue, int TestValueLength, long* NumericValue);
int ILibGetULong(const char *TestValue, const int TestValueLength, unsigned long* NumericValue);
int ILibFragmentText(char *text, int textLength, char *delimiter, int delimiterLength, int tokenLength, char **RetVal);
int ILibFragmentTextLength(char *text, int textLength, char *delimiter, int delimiterLength, int tokenLength);

/* Packet Methods */
struct packetheader *ILibCreateEmptyPacket();
void ILibAddHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength, char* FieldData, int FieldDataLength);
char* ILibGetHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength);
void ILibSetVersion(struct packetheader *packet, char* Version, int VersionLength);
void ILibSetStatusCode(struct packetheader *packet, int StatusCode, char* StatusData, int StatusDataLength);
void ILibSetDirective(struct packetheader *packet, char* Directive, int DirectiveLength, char* DirectiveObj, int DirectiveObjLength);
void ILibDestructPacket(struct packetheader *packet);
struct packetheader* ILibParsePacketHeader(char* buffer, int offset, int length);
int ILibGetRawPacket(struct packetheader *packet,char **buffer);
struct packetheader* ILibClonePacket(struct packetheader *packet);

/* Network Helper Methods */
int ILibGetLocalIPAddressList(int** pp_int);
#if defined(WINSOCK2)
	int ILibGetLocalIPAddressNetMask(int address);
	unsigned short ILibGetDGramSocket(int local, HANDLE *TheSocket);
	unsigned short ILibGetStreamSocket(int local, unsigned short PortNumber,HANDLE *TheSocket);
#elif defined(WINSOCK1) || defined(_WIN32_WCE)
	unsigned short ILibGetDGramSocket(int local, SOCKET *TheSocket);
	unsigned short ILibGetStreamSocket(int local, unsigned short PortNumber,SOCKET *TheSocket);
#else
	unsigned short ILibGetDGramSocket(int local, int *TheSocket);
	unsigned short ILibGetStreamSocket(int local, unsigned short PortNumber,int *TheSocket);
#endif

void* dbg_malloc(int sz);
void dbg_free(void* ptr);
int dbg_GetCount();

/* XML escaping methods */
int ILibXmlEscape(char* outdata, const char* indata);
int ILibXmlEscapeLength(const char* data);
int ILibInPlaceXmlUnEscape(char* data);

/* HTTP escaping methods */
int ILibHTTPEscape(char* outdata, const char* indata);
int ILibHTTPEscapeLength(const char* data);
int ILibInPlaceHTTPUnEscape(char* data);

/* Base64 handling methods */
int ILibBase64Encode(unsigned char* input, const int inputlen, unsigned char** output);
int ILibBase64Decode(unsigned char* input, const int inputlen, unsigned char** output);

/* Compression Handling Methods */
char* ILibDecompressString(unsigned char* CurrentCompressed, const int bufferLength, const int DecompressedLength);

#endif
