#ifndef __NAND63_H__
#define __NAND63_H__

/*
 * PL1063 NAND Controller Registers
 */

#define NAND63_REG_BASE		0x19400000
#define NAND63_REG_GENSET	(NAND63_REG_BASE + 0x00)
#define NAND63_REG_SM_TIMCAP	(NAND63_REG_BASE + 0x04)
#define NAND63_REG_COL		(NAND63_REG_BASE + 0x08)
#define NAND63_REG_ROW		(NAND63_REG_BASE + 0x0c)
#define NAND63_REG_XCNT		(NAND63_REG_BASE + 0x10)
#define NAND63_REG_DPORT	(NAND63_REG_BASE + 0x14)
#define NAND63_REG_CMD		(NAND63_REG_BASE + 0x18)
#define NAND63_REG_INT_ST	(NAND63_REG_BASE + 0x1c)
#define NAND63_REG_INT_MSK	(NAND63_REG_BASE + 0x20)
#define NAND63_REG_STATUS	(NAND63_REG_BASE + 0x24)
#define NAND63_REG_TEST1	(NAND63_REG_BASE + 0x28)
#define NAND63_REG_TEST2	(NAND63_REG_BASE + 0x2c)
#define NAND63_REG_ND_TIMCAP	(NAND63_REG_BASE + 0x30)
#define NAND63_REG_XD_TIMCAP	(NAND63_REG_BASE + 0x34)
#define NAND63_REG_PAD_CTL	(NAND63_REG_BASE + 0x38)
#define NAND63_REG_GPIO_CTL	(NAND63_REG_BASE + 0x3c)
#define NAND63_REG_GPIO_OUT	(NAND63_REG_BASE + 0x40)
#define NAND63_REG_GPIO_IN	(NAND63_REG_BASE + 0x44)
#define NAND63_REG_DMAC0	(NAND63_REG_BASE + 0x4c)
#define NAND63_REG_DMAC1	(NAND63_REG_BASE + 0x50)

/*
 * PL1063 NAND Controller General Setting Bitmaps
 */

#define SOFT_RST		(1 << 0)

#define REDUND_ENB		(1 << 1)
#define ECC_EN			(1 << 6)
#define TWO_CH_MODE		(1 << 8)
#define PIO_MODE		(1 << 9)
#define AUTO_STATUS		(1 << 11)
#define PAGE_CNT_EN		(1 << 13)

#define ND_WPJ			(1 << 2)
#define ND_CA_CYC		(1 << 3)
#define ND_RA_CYC		(1 << 4)

#define SM_WPJ			(1 << 14)
#define SM_CA_CYC		(1 << 19)
#define SM_RA_CYC		(1 << 20)
#define SM_PWR_EN		(1 << 16)

#define XD_WPJ			(1 << 15)
#define XD_CA_CYC		(1 << 21)
#define XD_RA_CYC		(1 << 22)
#define XD_PWR_EN		(1 << 17)

#define RDCMD_SEL		(1 << 5)
#define DEV_DW			(1 << 7)
#define AUTO_CP_MODE		(1 << 10)
#define CBACK_ECC_EN		(1 << 12)
#define SUSPEND			(1 << 18)
#define DUAL_MODE		(1 << 23)

/*
 * PL1063 NAND Controller Status - 70h command
 */

#define NAND63_ST_DPORT_RDY	(1 << 31)
#define NAND63_ST_CMD_END	(1 << 30)
#define NAND63_ST_STATUS_VALID	(1 << 29)
#define NAND63_ST_READY		(1 << 28)
#define NAND63_ST_WPJ		(1 << 7)
#define NAND63_ST_RDY		(1 << 6)
#define NAND63_ST_FAIL		(1 << 0)

/*
 * PL1063 NAND Controller Command Codes
 */

#define NAND63_CMD_READ		0x0000
#define NAND63_CMD_READ_2K	0x0030
#define NAND63_CMD_READOOB	0x5000
#define NAND63_CMD_PAGEPROG	0x8010
#define NAND63_CMD_CACHEPROG	0x8015
#define NAND63_CMD_ERASE	0x60d0
#define NAND63_CMD_STATUS	0x7000
#define NAND63_CMD_READID	0x9000
#define NAND63_CMD_RESET	0xff00
#define NAND63_CMD_RESET0	0x00ff
#define NAND63_CMD_RESET1	0x01ff
#define NAND63_CMD_RESET2	0x50ff

/*
 * PL1063 NAND Controller Interrupt
 */

#define NAND63_INT_CMD_DONE	(1 << 0)
#define NAND63_INT_ECC_ERR	(1 << 1)
#define NAND63_INT_TIME_OUT	(1 << 2)
#define NAND63_INT_SM_CD	(1 << 3)
#define NAND63_INT_XD_CD	(1 << 4)
#define NAND63_ST_SM_CDJ	(1 << 5)
#define NAND63_ST_XD_CDJ	(1 << 6)

#define NAND63_DEV_SM		0
#define NAND63_DEV_ND		1
#define NAND63_DEV_XD		7

#include <nand.h>
struct nand_chip *nand63_info (void);
int nand63_init (void);
int nand63_read_ecc (int page, int pc, void *data, void *rdnt);
int nand63_read_oob (int page, void *rdnt);
int nand63_write_ecc (int page, int pc, void *data, void *rdnt);
int nand63_write_oob (int page, void *rdnt);
int nand63_erase (int page);

#endif // __NAND63_H__
