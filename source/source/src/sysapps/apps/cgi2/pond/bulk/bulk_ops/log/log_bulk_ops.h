/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _LOG_BULK_OPS_H_
#define  _LOG_BULK_OPS_H_

#define LOG_DS_SYSLOG_SERVER_LEN		64
#define LOG_BULK_NAME 					"LOG"
#define SYSLOGD_PATH	"/var/syslogd_path"
enum
{
	SYSLOG_ERROR,      	/* Error log */
	SYSLOG_WARNING,    	/* Warning log */
	SYSLOG_NORMAL,     	/* Message log */
	SYSLOG_ALL, 		/* Internal debug */
};
struct LOG_DS
{
	int  log_mode;
	int  log_level;
	int  syslog_mode;
	char syslog_server[LOG_DS_SYSLOG_SERVER_LEN+1];
	int  syslog_port;
	int  ftplog_mode;
	int  smtplog_mode;
	int  systemlog_mode;
	int  imlog_mode;
};
int LOG_BULK_ReadDS(void* ds);
int LOG_BULK_SetDS(void* ds, char* group, char* item, char* value);
int LOG_BULK_CheckDS(void* ds, void* ds_org);
int LOG_BULK_WriteDS(void* ds, void* ds_org);
int LOG_BULK_RunDS(void* ds, void* ds_org);
int StartSyslogd(void);
int StopSyslogd(void);
#endif

