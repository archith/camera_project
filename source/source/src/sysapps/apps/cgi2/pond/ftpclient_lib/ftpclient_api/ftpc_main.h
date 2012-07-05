#ifndef _FTPC_API_H_
#define _FTPC_API_H_


typedef struct {
	char *server;
	char *user;
	char *passwd;
        char *path;
	int  port;  		
	int  passive;		// 0: passive off , 1: passive on
	char **filename;
	int f_total;		// total file amount
	char *lcd;		// local directory
	char* ftpcmd;
	int wait_file;
} ftp_t;

//-------------------------------------------------
//purpose: Upload file by ftp client
//input: ftp parameters
//return: FTP_OK - succeed
//        others - fail
//-------------------------------------------------
int ftpsend_start(ftp_t *info);

#define FTP_OK				0
#define FTP_SERVER_UNKNOWN		-1
#define FTP_LOGIN_FAILED		-2
#define	FTP_INVALID_PATH		-3
#define FTP_UPLOAD_FAILED		-4
#define FTP_TIMEOUT			-5

#endif
