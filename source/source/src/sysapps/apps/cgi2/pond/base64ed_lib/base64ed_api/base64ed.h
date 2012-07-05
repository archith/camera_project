#ifndef _SERCOMM_BASE64_DEF_
#define _SERCOMM_BASE64_DEF_

#include <stdio.h>
#include <fcntl.h>



#define SYSCFG_FN		"/mnt/ramdisk/system.conf"
#define EN_SYSCFG_FN	"def_sys.conf.en"
#define SECURITY_TAG	"[security]"
#define CHECKSUM_ATT	"checksum"


#define MAX_BUF_LEN		1024*8



int standard_encode64(char i_buf[], char o_buf[]);
int standard_decode64(char i_buf[], char o_buf[]);

int encode64(char i_buf[], char o_buf[]);
int decode64(char i_buf[], char o_buf[]);
int readFile2Buf(char fn[], char buf[]);
int writeBuf2File(char fn[], char buf[]);
int getRawData(char data[], char raw[]);
#endif
