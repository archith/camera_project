#ifndef __NOR_PPI_AMD_H__
#define __NOR_PPI_AMD_H__

/*
 * PL1029 NOR PPI AMD FUNCTIONS
 */
#include <nor.h>

int ppi_amd_init (void);
void amd_init(void);
void amd_reset(void);
int amd_read_id(void);
int amd_sector_erase(int addr);
int amd_dma_write(int addr,char *src,int size);
int amd_dma_read(int addr,char *dest,int size);
int amd_pio_write(int addr,char *src,int size);
int amd_pio_read(int addr,char *dest,int size);

#endif // __NOR_PPI_AMD_H__
