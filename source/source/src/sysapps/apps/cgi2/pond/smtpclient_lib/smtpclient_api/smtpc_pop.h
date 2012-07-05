#ifndef _SMTPC_POP_H_
#define _SMTPC_POP_H_

#define POP3_PORT	110

int smtpc_login_pop(email_t *pmail);

#define POP_LOGIN_OK		0
#define POP_SERVER_UNKNOWN	-1
#define POP_CONNECT_FAILED	-2
#define POP_USER_FAILED		-3
#define POP_PASSWORD_FAILED	-4
#define POP_LOGOUT_FAILED	-5

#endif // _SMTPC_POP_H_
