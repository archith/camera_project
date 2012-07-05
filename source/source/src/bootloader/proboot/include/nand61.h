#ifndef __NAND61_H__
#define __NAND61_H__

/*
 * NAND controller register
 */

#define NAND61_REG_IO_BASE	0x19400000
#define NAND61_REG_DATA		(NAND61_REG_IO_BASE + 0x00)
#define NAND61_REG_STATUS	(NAND61_REG_IO_BASE + 0x01)
#define NAND61_REG_CMD		(NAND61_REG_IO_BASE + 0x02)
#define NAND61_REG_INT_VEC	(NAND61_REG_IO_BASE + 0x03)
#define NAND61_REG_CTS		(NAND61_REG_IO_BASE + 0x04)
#define NAND61_REG_INT		(NAND61_REG_IO_BASE + 0x05)
#define NAND61_REG_CA0		(NAND61_REG_IO_BASE + 0x06)
#define NAND61_REG_CA1		(NAND61_REG_IO_BASE + 0x07)
#define NAND61_REG_PA0		(NAND61_REG_IO_BASE + 0x08)
#define NAND61_REG_PA1		(NAND61_REG_IO_BASE + 0x09)
#define NAND61_REG_PA2		(NAND61_REG_IO_BASE + 0x0A)
#define NAND61_REG_PC		(NAND61_REG_IO_BASE + 0x0B)
#define NAND61_REG_DT		(NAND61_REG_IO_BASE + 0x0C)

/*
 * SmartMedia command table
 */

#define NAND61_CMD_IF_D		0x00
#define NAND61_CMD_IF_E		0x01
#define NAND61_CMD_SW_RST	0x02
#define NAND61_CMD_DUMMY	0x03
#define NAND61_CMD_PIO_RD_T	0x04
#define NAND61_CMD_PIO_WR_T	0x05

#define NAND61_CMD_STATUS	0x10
#define NAND61_CMD_ID		0x13
#define NAND61_CMD_BERASE	0x14
#define NAND61_CMD_RESET	0x17

#define NAND61_CMD_PIO_RD	0x20
#define NAND61_CMD_PIO_RD_ECC	0x21
#define NAND61_CMD_DMA_RD	0x24
#define NAND61_CMD_DMA_RD_ECC	0x25
#define NAND61_CMD_DMA_RD_DATA	0x26

#define NAND61_CMD_PIO_WR	0x30
#define NAND61_CMD_PIO_WR_ECC	0x31
#define NAND61_CMD_DMA_WR	0x34
#define NAND61_CMD_DMA_WR_ECC	0x35
#define NAND61_CMD_DMA_WR_DATA	0x36

/*
 * PL1061 SmartMedia Status Register Bitmap
 */

#define NAND61_ST_WPJ             (1 << 7)
#define NAND61_ST_CARD_INSERTED   (1 << 6)
#define NAND61_ST_IDLE            (1 << 3)
#define NAND61_ST_TRANS_PD        (1 << 2)
#define NAND61_ST_DATAPORT_READY  (1 << 1)
#define NAND61_ST_FAIL            (1 << 0)



/*
 * Table of CTS (Cycle Time Scale)
 */

#define CTS_10_80_80		0xFB	/* 3351: 11_11_101_1 */
#define CTS_10_50_70		0x99	/* 2141: 10_01_100_1 */
#define CTS_15_90_90		0xA7	/* 2231: 10_10_011_1 */
#define CTS_15_60_60		0x54	/* 1120: 01_01_010_0 */
#define CTS_20_80_80		0x54	/* 1120: 01_01_010_0 */
#define CTS_20_60_80		0x44	/* 1020: 01_00_010_0 */
#define CTS_25_100_100		0x54	/* 1120: 01_01_010_0 */
#define CTS_25_50_75		0x02	/* 0010: 00_00_001_0 */
#define CTS_30_90_90		0x42	/* 1010: 01_00_001_0 */
#define CTS_30_60_90		0x02	/* 0010: 00_00_001_0 */
#define CTS_40_80_120		0x02	/* 0010: 00_00_001_0 */
#define CTS_66_133_133		0x00	/* 0000: 00_00_000_0 */

#define NAND61_DEV_SM		0
#define NAND61_DEV_ND		1

#include <nand.h>
struct nand_chip *nand61_info (void);
int nand61_init (void);
int nand61_read_ecc (int page, int pc, void *data, void *rdnt);
int nand61_read_oob (int page, void *rdnt);
int nand61_write_ecc (int page, int pc, void *data, void *rdnt);
int nand61_write_oob (int page, void *rdnt);
int nand61_erase (int page);

#endif // __NAND61_H__
