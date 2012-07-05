#ifndef __NOR_SPI_H__
#define __NOR_SPI_H__

/*
 * PL1029 NOR SPI FUNCTIONS
 */
#include <nor.h>

int spi_init (void);
int spi_read_id(void);
int spi_sector_erase(int addr);
int spi_dma_write(int addr,char *src,int size);
int spi_dma_read(int addr,char *dest,int size);
int spi_pio_write(int addr,char *src,int size);
int spi_pio_read(int addr,char *dest,int size);

#endif // __NOR_SPI_H__
