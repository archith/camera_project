#ifndef _EN_ACTION_H_
#define _EN_ACTION_H_

#include <cgi-parse.h>
int CreateEventServer(LIST* qs_list, int fd);
int CreateMotionEvent(LIST* qs_list, int fd);
int CreateIOEvent(LIST* qs_list, int fd);
int CreateMessageAction(LIST* qs_list, int fd);
int DeleteEventServer(LIST* qs_list, int fd);
int DeleteEvent(LIST* qs_list, int fd);
int DeleteEventAction(LIST* qs_list, int fd);
int Close(LIST* qs_list, int fd);
int Open(LIST* qs_list, int fd);
#endif

