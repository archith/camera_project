#ifndef _SMTPC_API_H_
#define _SMTPC_API_H_


#ifdef SMTPC_MAIN
	int smtpc_debug=0;
#else
	extern int smtpc_debug;
#endif



#define SENDMAIL_OK			0
#define SENDMAIL_TIMEOUT		-1
#define SENDMAIL_INPUT_INVALID		-2
#define SENDMAIL_UNKNOWN_HOST		-3
#define SENDMAIL_SOCK_FAILED		-4
#define SENDMAIL_CONN_FAILED		-5
#define SENDMAIL_SMTP_AUTH_FAILED	-6
#define SENDMAIL_POP_AUTH_FAILED	-7
#define SENDMAIL_SEND_FAILED		-8
#define SENDMAIL_RCPT_INVALID		-9
#define SENDMAIL_OTHER_FAILED		-10
#define SENDMAIL_ATTACH_FAILED		-11
#define SENDMAIL_ILLEGAL_SENDER		-12


#define MAX_RCPT_NUMBER		3	// max number of recipient
#define MAX_RCPT_LENGTH		64+1	// max address length of recipient
#define MAX_AT_NUMBER		110	// max number of attached file
#define MAX_AT_LENGTH		64+1	// max length of file name


typedef enum{
	AUTH_NONE		= 0,
	AUTH_SMTP		= 1,
	AUTH_POP_BEFORE_SMTP	= 2
}auth_e;


typedef struct{
	char *mailhost;         // smtp server
	int port;
	char *popserver;
	auth_e authtype;        // auth_e
	char *user;             // for smtp or pop auth
	char *pw;               // for smtp or pop auth
	char *from_addr;
	char to_addr[MAX_RCPT_NUMBER][MAX_RCPT_LENGTH];
	char *subject;
	int attach_num;
	char **attached;        //file path
	char *mesg;
	int wait_file;
        int smtp_ssl;
} email_t;


int smtpc_main(email_t* email);

#endif
