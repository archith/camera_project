#ifndef _CGI_TOOLS_H_
#define _CGI_TOOLS_H_

#include <kdef.h>
#define CGI_OUTPUT_FILE 			"/tmp/cgi_output_file"
#define CGI_MULTIPART_OUTPUT_FILE 	"/tmp/cgi_multipart_output_file"

int CGI_brace_left_lock_file(IN char* lock_file);
int CGI_brace_left(void);
int CGI_brace_right(void);
int CGI_multipart_retreive_file(IN char* name);
#endif

