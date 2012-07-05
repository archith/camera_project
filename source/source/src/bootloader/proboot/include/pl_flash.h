#ifndef PL_FLASH_H
#define PL_FLASH_H

int pl_flash_erase_range(unsigned int start_addr, unsigned int end_addr);
int pl_flash_write_v1(unsigned int offset, unsigned short *src, unsigned int num_bytes);

#endif
