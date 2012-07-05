#ifndef __NOR_H__
#define __NOR_H__

#define MAX_NOR_SECTORS 1024
#define BUFFER_SIZE 512*1024

extern int (*nor_sector_erase) (int addr);
extern int (*nor_dma_write) (int addr,char *src,int size);
extern int (*nor_dma_read) (int addr,char *dest,int size);
extern int (*nor_pio_write) (int addr,char *src,int size);
extern int (*nor_pio_read) (int addr,char *dest,int size);
extern int (*nor_read_id) (void);
extern int total_sector;
extern int buffer_addr;
extern unsigned char g_norinit; // after init flag
extern unsigned int g_nortype; // record nor flash typr for kernel
extern struct nor_sector norchip[MAX_NOR_SECTORS];
int nor_init (int type,char *memmap);
int nor2mem(int nor_addr,char *dest,int size);
int mem2nor(int nor_addr,char *src,int size);
int nor_dma_test(int nor_addr,char *src,int size,int total_times);

struct nor_sector {
	unsigned int	size; // bytes
	unsigned int	start_addr; // sector start address
};

/* TAG for kernel (xh_exec.c) */
#define MAX_SECT_SET    16
struct tg_norinfo {
    unsigned int  type;           /* 0: spi, 1: ppi-amd, 2: ppi-intel */
    struct  tg_sectinfo {
        unsigned short     size;
        unsigned short     quantum;
    } sectinfo[MAX_SECT_SET];
};
extern struct tg_norinfo g_norinfo;

// PL1029 NOR Flash Controller Register
//#define _1029_fpga
#ifndef _1029_fpga
   // pl1029 real addr
   #define BASE_NOR_FLASH (0x19780000)
   #define BASE_NOR_DIRECT (0x19800000)
#else
   // fpga addr
   #define BASE_NOR_FLASH (0x1A260000)
   #define BASE_NOR_DIRECT (0x1AC00000)
#endif //_1029_fpga

#define MAX_FLASH_WRITE 0 // without limited
#define MAX_FLASH_READ  0 // without limited
/*
 * nor flash controller register offset
 */
#define PIO_DATA_REG		BASE_NOR_FLASH+0x00
#define CONFIG_REG		BASE_NOR_FLASH+0x04
#define COM_REG			BASE_NOR_FLASH+0x08
#define INTR_S_REG		BASE_NOR_FLASH+0x0C //interrupt
#define NOR_ADDR_REG		BASE_NOR_FLASH+0x10
#define CTRL_ST_REG		BASE_NOR_FLASH+0x14 //status
#define INTR_M_REG		BASE_NOR_FLASH+0x18
#define SOFT_RST_REG		BASE_NOR_FLASH+0x1C
#define DMA_S_REG		BASE_NOR_FLASH+0x20
#define TIMER_REG		BASE_NOR_FLASH+0x24
#define PAD_REG			BASE_NOR_FLASH+0x28
#define PPI_PRO_CNT_REG		BASE_NOR_FLASH+0x2C
//#define PPI_TIMING_REG	BASE_NOR_FLASH+0x30
#define ADDR_CFG1		BASE_NOR_FLASH+0x34
#define ADDR_CFG2		BASE_NOR_FLASH+0x38
#define ADDR_CFG3		BASE_NOR_FLASH+0x3C
#define ADDR_CFG4		BASE_NOR_FLASH+0x40
#define ADDR_CFG5		BASE_NOR_FLASH+0x44
#define DATA_CFG1		BASE_NOR_FLASH+0x48 // command for SPI
#define DATA_CFG2		BASE_NOR_FLASH+0x4C
#define DATA_CFG3		BASE_NOR_FLASH+0x50
#define DATA_CFG4		BASE_NOR_FLASH+0x54
#define DATA_CFG5		BASE_NOR_FLASH+0x58

/*
 * nor flash controller interrupt bit (interrupt)
 */
#define INT_DMA			(1<<0)  // 0:command cycle done
#define INT_CMD			(1<<2)  // 0:command cycle done
#define INT_TIMEOUT		(1<<3)  // 0:time out
/*
 * nor flash controller status bit (polling)
 */
#define STATUS_DMA		(1<<0)  // 0:command cycle done
#define STATUS_CMD		(1<<2)  // 0:command cycle done
#define STATUS_TIMEOUT		(1<<3)  // 0:time out
#define STATUS_RB		(1<<4)  // 0:busy
/*
 * SPI NOR FLASH COMMAND , (MX25L4005,MX25L8005)
 */
#define MXIC_WREN		0x06  //write enable
#define MXIC_WRDI		0x04  //write disable
#define MXIC_RDID		0x9F  //read identification
#define MXIC_RDSR		0x05  //read status register
#define MXIC_WRSR		0x01  //write status register
#define MXIC_READ		0x03  //read data
#define MXIC_FASTREAD		0x0B  //fast read data
#define MXIC_SE			0x20  //sector erase
#define MXIC_BE			0xD8  //block erase
#define MXIC_CE			0xC7  //chip erase
#define MXIC_PP			0x02  //page program (1 word)
#define MXIC_DP			0xB9  //deep power down
#define MXIC_RDP		0xAB  //release from deep power down
#define MXIC_RES		0xAB  //read electronic id
#define MXIC_REMS		0x90  //read electronic manufacturer & device id
/*
 * PPI Write Operation Status Bits
 */
#define DATA_POLLING		128	//  IO_7
#define TOGGLE_BIT1		64	//  IO_6
#define EXCEEDED_TIMING		32	//  IO_5
#define SECTOR_ERASE_TIMER	8	//  IO_3
#define TOGGLE_BIT2		4	//  IO_2
/*
 *
 */
int wait_INTR(int bit,int val);
int wait_DMAC(void);
void disable_DMA(void);
#define MAXWAIT 0x02FFFFFF
extern int flash_byte_enable; // 1:Byte , 0:Word
extern int program_counter; //
void update_byte_enable(void);
void interrupt_mask(int mask);
void software_reset(void);
void disable_DMA(void);



#endif // __NOR_H__
