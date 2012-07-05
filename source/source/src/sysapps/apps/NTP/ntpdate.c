#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ntp.h"

#ifdef _ONEDAY_
#define CheckTime 86400
#else
#define CheckTime 64
#endif
#define SRV_LEN	  64
#define IP_LEN	  16

static void init(void)
{
	int pid;

	if ((pid = fork()) < 0) {
		MYPRINTF("Program fork fail\n");
		_exit(-1);
	} else if (pid != 0) {
		_exit(0);	/* parent goes bye-bye */
	}
				/* child continues */
	setsid();		/* standard daemon procedure */
	chdir("/");
	umask(0);
}

static void GetServer(char *server){
	
	PRO_GetStr(SEC_SYS, SYS_NTP_SERVER, server, SRV_LEN);	
	return;
}

int main(int argc,char *argv[]){

	struct hostent *h;
	char addr[IP_LEN]={0};
	int i=0;
 	char server[SRV_LEN]={0};

	memset(server,0,sizeof(server));
	if(argc == 1) {
		GetServer(server);
	} else if(argc == 2){
 	  	strcpy(server,argv[1]);
	} else	{
		exit(1);	
	}	

	if(!(h=gethostbyname(server))){
		AddLog("NTP",SYSLOG_WARNING,2);
		exit(1);
	}
	strcpy(addr,inet_ntoa(*((struct in_addr*)h->h_addr)));

	init();			/* daemon init */

	for( ; ;i++) {

		if((i == CheckTime) || (i == 0))
		{
			MYPRINTF("starting NTP sync\n");
			NTP_update(addr);
			i = 0;
		}
		sleep(1);
	}

	return 0;
}
