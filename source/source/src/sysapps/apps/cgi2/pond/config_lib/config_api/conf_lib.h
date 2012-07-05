#ifndef _SERCOMM_CONFIG_MANIPULATE_DEF_
#define _SERCOMM_CONFIG_MANIPULATE_DEF_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>

#define SYS_CONFIG_PATH				"/etc/system.conf"
#define DECODED_CONFIG_TEMP_PATH	"/tmp/temp.decoded.conf"

#define CONF_ERROR	-1
#define CONF_OK		0

//debug purpose 
//#define _CONF_API_DEBUG_

// read the system.conf and save the encoded context to the designated file
int read_encoded_conf(char *fn);
// decode the designated file and copy to the system.conf
int save_encoded_conf(char *fn);
#endif
