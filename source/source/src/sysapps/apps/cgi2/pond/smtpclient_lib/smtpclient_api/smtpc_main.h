#ifndef _SMTPC_MAIN_H_
#define _SMTPC_MAIN_H_

#include "smtpc_var.h"

int smtpc_connect(email_t* email, SMTPC_VAR* var);
int smtpc_setup_envelope(email_t* email, SMTPC_VAR* var);
int smtpc_setup_body(email_t* email, SMTPC_VAR* var);
int smtpc_disconnect(SMTPC_VAR* var);

#endif

