#ifndef _SMTPC_UTIL_H_
#define _SMTPC_UTIL_H_

#include "smtpc_var.h"


//int get_response(FILE* fp);
//int communicate(COMMUNICATE* com, char *str, ...);
//int write_data(COMMUNICATE* com, char* buf, size_t size);

int smtpc_send_message(COMMUNICATE* com);
int smtpc_recv_message(COMMUNICATE* com);
int smtpc_communicate(COMMUNICATE* com, char* str, ...);
int smtpc_upload(COMMUNICATE* com, char* str, ...);

int base64Encode(struct smtpc_buf* in, struct smtpc_buf* out);
int quoted_printable_encoder(struct smtpc_buf* in, struct smtpc_buf* out);


#endif

