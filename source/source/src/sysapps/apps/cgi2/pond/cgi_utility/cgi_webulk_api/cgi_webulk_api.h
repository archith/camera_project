#ifndef _CGI_WEBULK_API_H_
#define _CGI_WEBULK_API_H_

#include <kdef.h>
#include <cgi-parse.h>
#include <webulk_ops.h>
#include <bulk_ops.h>
int CGI_webulk_save(IN struct bulk_ops** bulk_ops_list, IN struct webulk_ops** webulk_ops_list, IN LIST* list, IN int auth_flag);
int CGI_webulk_get(IN struct bulk_ops** bulk_ops_list, IN struct webulk_ops** webulk_ops_list, IN char* path, IN int auth_flag);

#endif

