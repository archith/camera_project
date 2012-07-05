/***************************************************************
*             Copyright (C) 2007 by SerComm Corp.
*                    All Rights Reserved.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef _SMTPC_MIME_H_
#define _SMTPC_MIME_H_

#include "smtpc_var.h"

int email_body_header(email_t *email, SMTPC_VAR* var);
void email_mime_header(SMTPC_VAR* var);


enum{
	POS_MIDDLE,
	POS_END
};
void email_mime_boundary(int position, SMTPC_VAR* var);

int email_body_message(char* message, SMTPC_VAR* var);
int email_attach_one_file(char* fpath, SMTPC_VAR* var);

#endif

