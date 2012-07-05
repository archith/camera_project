#ifndef _BASE_TOOLS_H_
#define _BASE_TOOLS_H_

#include <unistd.h>
void BASE_pre_time();
void BASE_post_time();
int BASE_check_FWImage(char* upg_path);
int BASE_read_mac(unsigned char* eth_addr);
int system2(const char *command, ...);
#endif

