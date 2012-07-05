#ifndef _LOG_API_H_
#define _LOG_API_H_
#include <log_bulk_ops.h>
#define SYSTEMLOG_FILE	"/var/nc.log"
int AddLog(char *task,int level, int index,...);
int ClearLog(void);

#define LOGFILE_SECTION_HTTP			"http"
#define LOGFILE_SECTION_RTSP			"rtsp"
#define MSGID_START_TO_VIEW_VIDEO		1
#define MSGID_STOP_VIEWING_VIDEO		2

#endif
