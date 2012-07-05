#ifndef _MOT_DEBUG_H_
#define _MOT_DEBUG_H_

extern char logmsg[128];
extern void LogMsg(char *s);
#ifdef _DEBUG_
#define MPRINTF(format, argument...) printf(format , ## argument);
#else
#define MPRINTF(format, argument...)
#endif

#endif
