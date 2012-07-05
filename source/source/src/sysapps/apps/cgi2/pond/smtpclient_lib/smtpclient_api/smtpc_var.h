#ifndef	_SMTPC_VAR_H_
#define _SMTPC_VAR_H_
#include "smtpc_api.h"

//#define _SMTPC_VERBOSE_
//#define _SMTPC_DEBUG_
#ifdef _SMTPC_DEBUG_

	#define DB(fmt, arg...);	\
		{\
			fprintf(stderr, fmt, ##arg);\
		}

#else

	#define DB(fmt, arg...);	{}

#endif


typedef struct smtpc_buf{
	char* buf;
	size_t size;
	size_t used;
}SMTPC_BUF;


typedef struct communicate{
	int sendfd;
	int recvfd;
	SMTPC_BUF* bufp;
	int req_auth;
	int verbose;
	int debug;

	//FILE* sendfp;
	//FILE* recvfp;
	//int need_reply;	//1-yes, 0-no
}COMMUNICATE;

typedef struct bad_recipients{
	char* addr_ptr[MAX_RCPT_NUMBER];
}BAD_RECIPIENTS;

//typedef struct mail_attribute{
//	int used_mime;
//}ATTRIBUTE;

typedef struct smtpc_var{
	COMMUNICATE com;
	BAD_RECIPIENTS bad_rcpt;
	int attach_from_mem;	// attach file from memory
}SMTPC_VAR;





#endif	// _SMTPC_VAR_H_
