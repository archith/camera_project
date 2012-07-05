#ifndef _MULTIPART_H_
#define _MULTIPART_H_

#include <kdef.h>
int multipart_retreive_form_data(IN char* name, INOUT char** buf, INOUT unsigned int* buf_len);

#endif

