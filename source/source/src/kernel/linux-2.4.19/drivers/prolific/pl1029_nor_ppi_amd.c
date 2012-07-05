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
#include <linux/config.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/dma.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/setup.h>

#include "pl1029_nor.h"

static unsigned long ppi_write_wait_count = 0x500;
static unsigned long ppi_erase_wait_count = 0x1050000;

int ppi_amd_init(void)
{
	return 0;
}

int ppi_amd_reset(void)
{
	writel(0xf0, PIO_DATA_REG);
	
	return 0;
}

int ppi_amd_read_id(u8 *id)
{
	writel(0xAAA, NOR_ADDR_REG);
	writel(0xAA, PIO_DATA_REG);
	writel(0x555, NOR_ADDR_REG);
	writel(0x55, PIO_DATA_REG);
	writel(0xAAA, NOR_ADDR_REG);
	writel(0x90, PIO_DATA_REG);
	writel(0x02, NOR_ADDR_REG);
	*id=readb(PIO_DATA_REG);
	ppi_amd_reset();

	return 0;
}

int ppi_amd_cmd_erase(unsigned long addr, int mode)
{
	int rc = 0;
	unsigned char stat;

	switch ( mode ) {
	case DMA_START:
		writel(ppi_erase_wait_count, PPI_PRO_CNT_REG);

		/* write-protected disable, auto-mode, 6 sets of commands */
		writel(0x36, COM_REG); 
	
		writel(0xAAA, ADDR_CFG1);
		writel(0xAA,  DATA_CFG1);
		writel(0x555, ADDR_CFG2);
		writel(0x55,  DATA_CFG2);
		writel(0xAAA, ADDR_CFG3);
		writel(0x80,  DATA_CFG3);
		writel(0xAAA, ADDR_CFG4);
		writel(0xAA,  DATA_CFG4);
		writel(0x555, ADDR_CFG5);
		writel(0x55,  DATA_CFG5);
	
		writel(addr, NOR_ADDR_REG);
		writeb(0x30, PIO_DATA_REG);
		break;
	case DMA_END:
		stat = readb(PIO_DATA_REG)&0x80; /* DATA_POLLING 0x80 */
		if ( stat == 0 )
			rc = -1;
		else
			rc = 0;
		writel(0x00, COM_REG);
		break;
	}
	
	return rc;
}

int ppi_amd_cmd_write(unsigned long addr, int size, int mode)
{
	int rc = 0;
	
	switch ( mode ) {
	case DMA_START:
		writel(ppi_write_wait_count, PPI_PRO_CNT_REG);

		/* write-protected disable, auto-mode, 4 sets of commands */
		writel(0x34, COM_REG); 
	
		writel(0xAAA, ADDR_CFG1);
		writel(0xAA,  DATA_CFG1);
		writel(0x555, ADDR_CFG2);
		writel(0x55,  DATA_CFG2);
		writel(0xAAA, ADDR_CFG3);
		writel(0xA0,  DATA_CFG3);
	
		writel(addr, NOR_ADDR_REG);
		writel(0x80000000|size, DMA_S_REG); 
		break;
	case DMA_END:
		writel(0x00, COM_REG);
		break;
	}
	                                         
	return rc;
}

int ppi_amd_cmd_read(unsigned long addr, int size, int mode)
{
	int rc = 0;
	
	switch ( mode ) {
	case DMA_START:
		writel(addr, NOR_ADDR_REG);
		writel(0x40000000|size, DMA_S_REG);
		break;
	case DMA_END:
		/* nothing to do */
		break;
	}
	
	return rc;
}

pl1029_nor_ops ppi_amd = {
	.init =		ppi_amd_init,
	.reset =	ppi_amd_reset,
	.read_id =	ppi_amd_read_id,
	.cmd_erase =	ppi_amd_cmd_erase,
	.cmd_write =	ppi_amd_cmd_write,
	.cmd_read =	ppi_amd_cmd_read,
};

