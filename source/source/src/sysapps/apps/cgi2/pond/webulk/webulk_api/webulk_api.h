#ifndef _WEBULK_API_H_
#define _WEBULK_API_H_
#include <kdef.h>
#include <webulk_ops.h>
#include <bulk_ops.h>
int webulk_save(IN struct bulk_ops** bulk_ops_list, IN struct webulk_ops** webulk_ops_list, IN char* item, IN char* value,  IN int auth_flag);
int webulk_get(IN struct bulk_ops** bulk_ops_list, IN struct webulk_ops** webulk_ops_list, IN char* item,  IN int auth_flag);
#endif

