#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include<cross.h>
#define  UPGRADE_START    BSPCONF_KERNEL_FLASH_OFFSET     //
#define  UPGRADE_END       (FLASH_SIZE-1)
#define  ERASEALL_START    BSPCONF_CONF_FLASH_OFFSET

int chksum_verify(void);
int mac_verify(void);
int Download(char* extra_buf, int extra_buf_len);
void to_download(int dnl);
void getPIDfromFlash(void);
#endif
