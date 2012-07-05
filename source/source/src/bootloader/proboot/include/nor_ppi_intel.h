#ifndef __NOR_PPI_INTEL_H__
#define __NOR_PPI_INTEL_H__

/*
 * PL1029 NOR PPI INTEL FUNCTIONS
 */
#include <nor.h>

int ppi_intel_init (void);
void intel_init(void);
void intel_reset(void);
int intel_read_id(void);
int intel_sector_erase(int addr);
int intel_dma_write(int addr,char *src,int size);
int intel_dma_read(int addr,char *dest,int size);
int intel_pio_write(int addr,char *src,int size);
int intel_pio_read(int addr,char *dest,int size);


#endif // __NOR_PPI_INTEL_H__
