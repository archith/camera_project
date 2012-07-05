/*
 *  Copyright (C) 2006-2008 Prolific Technology Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef I2C_ALGO_61_H
#define I2C_ALGO_61_H 1

#include <linux/i2c.h>

/* Example of a sequential read request:
	struct i2c_61_msg s_msg;

	s_msg.addr=device_address;
	s_msg.len=length;
	s_msg.buf=buffer;
	s_msg.waddr=word_address;
	ioctl(file,I2C_SREAD, &s_msg);
 */
#define I2C_SREAD	0x780	/* SREAD ioctl command */

struct i2c_61_msg {
	__u16 addr;	/* device address */
	__u16 waddr;	/* word address */
	short len;	/* msg length */
	char *buf;	/* pointer to msg data */
};

struct i2c_algo_61_data {
	void *data;		/* private data for lolevel routines	*/
    void (*writew) (void *adap_data, int reg, int val);
    void (*writel) (void *adap_data, int reg, int val);
    int  (*readw)  (void *adap_data, int reg);
    int  (*readl)  (void *adap_data, int reg);
	int  (*getown) (void *adap_data);
	int  (*getclock) (void *adap_data);
	void (*wait_for_pin) (void *adap_data, int *status, int timeout);

	/* local settings */
	int timeout;
};

int i2c_61_add_bus(struct i2c_adapter *);
int i2c_61_del_bus(struct i2c_adapter *);

/* PL1061 I2C Register offset */
#define I2C_GCR     0
#define I2C_MCR     4
#define I2C_SCR     8

/*
 * Prolific i2c register offset
 */

#define DEF_I2C_TIMEOUT     (HZ/2)

/* General Control Register (GCR) */
#define PL_GCR_I2CEN                (1L << 31)
#define PL_GCR_GIOEN                (1L << 30)
#define PL_GCR_MCR_IEN              (1L << 29)
#define PL_GCR_SCR_IEN              (1L << 28)
#define PL_GCR_KDV(xclk)            ((xclk & 0x3f) << 16)
#define PL_GCR_XSCL_PU              (1L << 15)
#define PL_GCR_XSCL_2MA             (0x00 << 12)
#define PL_GCR_XSCL_6MA             (0x01 << 12)
#define PL_GCR_XSCL_10MA            (0x02 << 12)
#define PL_GCR_XSCL_14MA            (0x03 << 12)
#define PL_GCR_SCL_E                (1L << 11)
#define PL_GCR_SCL_O                (1L << 10)
#define PL_GCR_XSCL_I               (1L << 8)
#define PL_GCR_XSDA_PU              (1L << 7)
#define PL_GCR_XSDA_2MA             (0x00 << 4)
#define PL_GCR_XSDA_6MA             (0x01 << 4)
#define PL_GCR_XSDA_10MA            (0x02 << 4)
#define PL_GCR_XSDA_14MA            (0x03 << 4)
#define PL_GCR_SDA_E                (1L << 3)
#define PL_GCR_XSDA_I               (0x01)

/* Master Control Register (MCR) */
#define PL_MCR_ADDR(ten_bit, addr)  (((((ten_bit) ? 1L : 0) << 10) | (addr)) << 16)
#define PL_MCR_STOP                 ((0x0) << 13)
#define PL_MCR_CMD_WR               ((0x4) << 13)
#define PL_MCR_CMD_RD               ((0x5) << 13)
#define PL_MCR_WR_DATA              ((0x6) << 13)
#define PL_MCR_RD_NEXT              ((0x7) << 13)

#define PL_MCR_STATUS(x)            (x & ((0x7) << 8))
#define     PL_MCR_STATUS_ACK       ((0x6) << 8)
#define     PL_MCR_STATUS_NO_ACK    ((0x4) << 8)
#define     PL_MCR_STATUS_ERR       ((0x5) << 8)
#define PL_MCR_ACTIVE               (1L << 11)
#define PL_MCR_INT                  (1L << 12)
#define PL_MCR_DATA_MASK            (0xff)

/* Slave Control Register (SCR) */
#define PL_SCR_SEN                  (1L << 31)      /* Slave Mode Enable */
#define PL_SCR_GEN                  (1L << 30)      /* Slave Mode General Call Enable */
#define PL_SCR_ADDR(ten_bit, addr)  ((((ten_bit) ? 1L : 0)<< 10) | (addr)) << 16)

#define PL_SCR_RSP_NO_RSP           (0x0 << 14)
#define PL_SCR_RSP_NO_ACK           (0x2 << 14)
#define PL_SCR_RPS_ACK              (0x3 << 14)

#define PL_SCR_STATUS_MASK          ((0x7) << 8)
#define     PL_SCR_STATUS_CALL_REQ  ((0x5) << 8)
#define     PL_SCR_STATUS_RD_REQ    ((0x6) << 8)
#define     PL_SCR_STATUS_WR_REQ    ((0x4) << 8)
#define     PL_SCR_STATUS_STOP      ((0x7) << 8)



#endif /* I2C_ALGO_61_H */
