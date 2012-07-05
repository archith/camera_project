#ifndef _CGI_WEB_HTML_H_
#define _CGI_WEB_HTML_H_

#include <bulk_ops.h>
#include <webulk_ops.h>
#include <all_bulk_ops.h>
#include <all_webulk_ops.h>

#define CGI_WEB_HTML_INIT() \
	struct webulk_ops* all_webulk_ops_list[]= ALL_WEBULK_OPS_LIST;\
	struct bulk_ops* all_bulk_ops_list[] = ALL_BULK_OPS_LIST;

#define CGI_WEB_HTML_BACK(html)	\
	CGI_WEB_HTML_back(all_bulk_ops_list, all_webulk_ops_list, html)

#define CGI_WEB_HTML_BACK_MESSAGE(html, type, group, item...) \
	CGI_WEB_HTML_back_message(all_bulk_ops_list, all_webulk_ops_list, html, type, group, ##item)
	
int CGI_WEB_HTML_back(struct bulk_ops** bulk_ops_list, struct webulk_ops** webulk_ops_list, char* html);
int CGI_WEB_HTML_back_message(struct bulk_ops** bulk_ops_list, struct webulk_ops** webulk_ops_list, char* html, int type, char* group, char* item, ...);

#endif

