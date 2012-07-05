#ifndef _ADM_LOCK_H_
#define _ADM_LOCK_H_

#include <netdb.h>
#include <time.h>
#include "base.h"

#define ADM_GROUP_NUM	2

typedef struct{
	char last_user[32+1];
	char last_ip[NI_MAXHOST+1];
	time_t last_time;
}ADM_LOCK;

int adm_lock(server* srv, connection* con, void* p_d);
int response_server_busy(server* srv, connection* con);

#endif //_ADM_LOCK_H_
