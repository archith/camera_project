/***************************************************************
*             Copyright (C) 2003 by SerComm Corp.
*                    All Rights Reserved.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef SSDPPARSER_H
#define SSDPPARSER_H

/* copy from upnp.h */
#define LINE_SIZE	280
#ifdef DEBUG
#define DBGONLY(x) x	
#define DBG(x) x
#else
#define DBGONLY(x)
#define DBG(x)
#endif


/* copy from upnpapi.h */
typedef enum CmdType{ERROR=-1,OK,ALIVE,BYEBYE,SEARCH,NOTIFY,TIMEOUT} Cmd;
typedef enum SEARCH_TYPE{SERROR=-1,ALL,ROOTDEVICE,DEVICE,DEVICETYPE,SERVICE} SearchType;
typedef enum SsdpCmdType{SSDP_ERROR=-1,SSDP_OK,SSDP_ALIVE,SSDP_BYEBYE,
                         SSDP_SEARCH,SSDP_NOTIFY,SSDP_TIMEOUT} CmdT;
typedef enum SsdpSearchType{SSDP_SERROR=-1,SSDP_ALL,SSDP_ROOTDEVICE,
                           SSDP_DEVICE,SSDP_DEVICETYPE,SSDP_SERVICE} SType;


typedef struct SsdpEventStruct
{
    enum CmdType Cmd;
    enum SEARCH_TYPE RequestType;
    int  ErrCode;
    int  MaxAge;
    int  Mx;
    char UDN[LINE_SIZE];
    char DeviceType[LINE_SIZE];
    char ServiceType[LINE_SIZE];  //NT or ST
    char Location[LINE_SIZE];
    char HostAddr[LINE_SIZE];
    char Os[LINE_SIZE];
    char Ext[LINE_SIZE];
    char Date[LINE_SIZE];
    char Man[LINE_SIZE];
    struct sockaddr_in * DestAddr;
    void * Cookie;
} Event;



/* from ssdpparser.c */
void InitParser(void);
int AnalyzeCommand(char * szCommand, Event * Evt);
//SsdpFunPtr CallBackFn;

/* copy from ssdplib.h */
//	compiler not allow declaration like this
//	typedef int (*ParserFun)((char *)NULL, (Event *)NULL);
//typedef int (*ParserFun)(char *Cmd, Event *Evt);
typedef int (*ParserFun)(char *, Event *);


// For Parser
#define NUM_TOKEN  12
//Constant
//#define	 BUFSIZE   2500
//#define	 BUFSIZE   512
//#define	 BUFSIZE   250
//#define  SSDP_IP   "239.255.255.250"
//#define  SSDP_PORT 1900
//#define  NUM_TRY 3
//#define  NUM_COPY 2
//#define  THREAD_LIMIT 50
//#define  COMMAND_LEN  300

//Error code
#define NO_ERROR_FOUND    0
#define E_REQUEST_INVALID  	-3
#define E_RES_EXPIRED		-4
#define E_MEM_ALLOC		-5
#define E_HTTP_SYNTEX		-6
#define E_SOCKET 		-7
#define RQST_TIMEOUT    20


#endif
