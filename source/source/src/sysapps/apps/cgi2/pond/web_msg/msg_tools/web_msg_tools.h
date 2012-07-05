#ifndef _WEB_MSG_TOOLS_H_
#define _WEB_MSG_TOOLS_H_
#include <kdef.h>
#include <web_msg.h>
enum
{
	MSG_TYPE_POPUP,
	MSG_TYPE_SYS_MSG,
};
int WebMsg_GetStr(IN char* group, IN char* item, OUT char* value, IN int value_len);

#endif

