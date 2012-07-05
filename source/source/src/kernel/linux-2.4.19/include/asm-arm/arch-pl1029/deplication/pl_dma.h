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
/* 
 * NOTE: all this is true *only* for PL- internal peripheral on Mips boards
 */

#ifndef __ASM_MIPS_PL_DMA_H
#define __ASM_MIPS_PL_DMA_H

#include <linux/config.h>
#include <linux/spinlock.h>		/* And spinlocks */
#include <linux/delay.h>

#include <asm/io.h>			/* need byte IO */
#include <asm/system.h>

// Modified by chun-wen 2004-07-09 for pl1091 platform
#include <asm/pl_reg.h>

//#include <asm/pl1061/pl1061.h>

/* 
 * PL1061 DMA Channel definition
 */

#define DMA_PL_SM_DATA      0
#define DMA_PL_SM_RDNT      1
#define DMA_PL_AC97_OUT     2
#define DMA_PL_AC97_IN      3
#define DMA_PL_ADC          4
#define DMA_PL_SD           5
#define DMA_PL_CF           6
#define DMA_PL_UART_IN      7
#define DMA_PL_UART_OUT     8
#define DMA_PL_MS           9


#define MAX_DMA_CHANNELS	16
#define MAX_DMA_MDE         4
#define MAX_DMA_MDE_COUNT   ((1 << 21) - 16)
/*
 * The maximum address in KSEG0 that we can perform a DMA transfer to on this
 * platform.  This describes only the PC style part of the DMA logic like on
 * Deskstations or Acer PICA but not the much more versatile DMA logic used
 * for the local devices on Acer PICA or Magnums.
 */
#define MAX_DMA_ADDRESS		0x1000000  //(PAGE_OFFSET + 0x01000000)

/* 8237 DMA controllers */
//#define IO_DMA_BASE	    (0x197C0000)
//Modified by chun-wen 2004-07-09 For pl1091 platform .
#define IO_DMA_BASE	    0xE97c0000
//VIRT_IO_ADDRESS(0x197C0000) //KSEG1 | 

/* DMA controller registers */
#define DMA_IDX_REG         0x00    /* MDE index register */
#define DMA_DIR_REG         0x02    /* Count direction. Cont down (1), Count up (0)  */
#define DMA_CMD_REG         0x03    /* Channel command register */
#define DMA_MDT_REG         0x40    /* MDT header register (r/w) */
#define DMA_CNT_REG         0x7C    /* Control Register (r/w) */


#define DMA_CHN_ENABLE      (1 << 7)
#define DMA_MODE_UPSTREAM   (1 << 6)
#define DMA_BURST_LINE      (1 << 3)
#define DMA_PRI_HIGH        (1 << 2)
#define DMA_PACK_ENABLE     (1)


#define DMA_MODE_READ	    1    	/* I/O to memory, no autoinit, increment, single mode */
#define DMA_MODE_WRITE	    0    	/* memory to I/O, no autoinit, increment, single mode */

typedef struct dma_mdt {
    unsigned int saddr;     /* starting address */
    unsigned int mde;       /* a mde flag */
} dma_mdt_t;

struct dma_chan {
	int  lock;
	const char *device_id;
    dma_mdt_t  *mdt;
    dma_mdt_t  mdt_default[MAX_DMA_MDE];
};

extern spinlock_t  dma_spin_lock;

static __inline__ unsigned char dma_readb(unsigned int reg, unsigned int dmanr)
{
    //printk("dma_readb,call readb(port=%x\n",IO_DMA_BASE + reg + sizeof(int) * dmanr);
    return readb((unsigned char *)(IO_DMA_BASE + reg + sizeof(int) * dmanr));
}

static __inline__ void dma_writeb(unsigned int reg, unsigned int dmanr, unsigned char value)
{
    //printk("dma_writeb,call writeb(value=%x,port=%x\n",value,IO_DMA_BASE + reg + sizeof(int) * dmanr);
    writeb(value, (unsigned char *)(IO_DMA_BASE + reg + sizeof(int) * dmanr));
}

static __inline__ unsigned int dma_readl(unsigned int reg, unsigned int dmanr)
{
    return readl((unsigned int *)(IO_DMA_BASE + reg + sizeof(int) * dmanr));
}

static __inline__ void dma_writel(unsigned int reg, unsigned int dmanr, unsigned int value)
{
    writel(value, (unsigned int *)(IO_DMA_BASE + reg + sizeof(int) * dmanr));
}


static __inline__ unsigned long claim_dma_lock(void)
{
	unsigned long flags;
	spin_lock_irqsave(&dma_spin_lock, flags);
	return flags;
}

static __inline__ void release_dma_lock(unsigned long flags)
{
	spin_unlock_irqrestore(&dma_spin_lock, flags);
}

/* enable/disable a specific DMA channel */
static __inline__ void enable_dma(unsigned int dmanr)
{
    unsigned char val;

    //printk("pl_dma.h,enable_dma(dmanr=%d)\n",dmanr);	
    if (dmanr >= MAX_DMA_CHANNELS) {
        printk("%s:invalid dmanr %d\n", __FUNCTION__, dmanr);
        return;
    }

    val = dma_readb(DMA_CMD_REG, dmanr);
    dma_writeb(DMA_CMD_REG, dmanr, val | DMA_CHN_ENABLE);
}

static __inline__ void disable_dma(unsigned int dmanr)
{
    unsigned char val;

    //printk("pl_dma.h,disable_dma(dmanr=%d)\n",dmanr);
    if (dmanr >= MAX_DMA_CHANNELS) {
        printk("%s:invalid dmanr %d\n", __FUNCTION__, dmanr);
        return;
    }
    //printk("pl_dma.h,disable_dma(2)DMA_CMD_REG=%x\n",DMA_CMD_REG);	
    val = dma_readb(DMA_CMD_REG, dmanr);
    dma_writeb(DMA_CMD_REG, dmanr, val & ~DMA_CHN_ENABLE);
}

/*
 *  do nothing for pl- dma
 */
static __inline__ void clear_dma_ff(unsigned int dmanr)
{
}

/* set mode (above) for a specific DMA channel */
static __inline__ void set_dma_mode(unsigned int dmanr, char mode)
{
    unsigned char val;

    //printk("pl_dma.h,set_dma_mode(dmanr=%d)\n",dmanr);	
    if (dmanr >= MAX_DMA_CHANNELS)
        return;

    val = dma_readb(DMA_CMD_REG, dmanr);
    if (mode) /* DMA_MODE_READ */
        val |= DMA_MODE_UPSTREAM;
    else
        val &= ~DMA_MODE_UPSTREAM;

    dma_writeb(DMA_CMD_REG, dmanr, val);

}

/* Set only the page register bits of the transfer address.
 * This is used for successive transfers when we know the contents of
 * the lower 16 bits of the DMA current address register, but a 64k boundary
 * may have been crossed.
 */
static __inline__ void set_dma_page(unsigned int dmanr, char pagenr)
{
#if 0
	switch(dmanr) {
		case 0:
			dma_outb(pagenr, DMA_PAGE_0);
			break;
		case 1:
			dma_outb(pagenr, DMA_PAGE_1);
			break;
		case 2:
			dma_outb(pagenr, DMA_PAGE_2);
			break;
		case 3:
			dma_outb(pagenr, DMA_PAGE_3);
			break;
		case 5:
			dma_outb(pagenr & 0xfe, DMA_PAGE_5);
			break;
		case 6:
			dma_outb(pagenr & 0xfe, DMA_PAGE_6);
			break;
		case 7:
			dma_outb(pagenr & 0xfe, DMA_PAGE_7);
			break;
	}
#endif
}




/* Get DMA residue count. After a DMA transfer, this
 * should return zero. Reading this while a DMA transfer is
 * still in progress will return unpredictable results.
 * If called before the channel has been used, it may return 1.
 * Otherwise, it returns the number of _bytes_ left to transfer.
 *
 * Assumes DMA flip-flop is clear.
 */
static __inline__ int get_dma_residue(unsigned int dmanr)
{
    unsigned char val;
    val = dma_readb(DMA_CMD_REG, dmanr);

    return (val & DMA_CHN_ENABLE) ? 1 : 0;
}


extern int request_dma(unsigned int dmanr, const char * device_id);	/* reserve a DMA channel */
extern void free_dma(unsigned int dmanr);	/* release it again */
extern void set_dma_addr(unsigned int dmanr, unsigned int a);
extern void set_dma_count(unsigned int dmanr, unsigned int count);
/* #001 dma advanced function, those function is exculsive with set_dma_addr & */
/*      set_dma_count. By the way the mde_vaddr must be logical address */
extern void set_dma_mde_base(unsigned int dmanr, const char *mde_vaddr);

static __inline__ void set_dma_mde_idx(unsigned int dmanr, unsigned int mde_idx)
{
    if (dmanr >= MAX_DMA_CHANNELS) {
        printk("%s:invalid dmanr %d\n", __FUNCTION__, dmanr);
        return;
    }
    if (mde_idx > 255) {
        printk("%s:dmanr %d invalide mde_idx %d\n" ,__FUNCTION__, dmanr, mde_idx);
        return;
    }

    dma_writeb(DMA_IDX_REG, dmanr, mde_idx);

}
static __inline__ const char *get_dma_mde_base(unsigned int dmanr)
{
    if (dmanr >= MAX_DMA_CHANNELS) {
        printk("%s:invalid dmanr %d\n", __FUNCTION__, dmanr);
        return NULL;
    }

    return bus_to_virt(dma_readl(DMA_MDT_REG, dmanr));
}
static __inline__ int  get_dma_mde_idx(unsigned int dmanr)
{
    if (dmanr >= MAX_DMA_CHANNELS) {
        printk("%s:invalid dmanr %d\n", __FUNCTION__, dmanr);
        return -1;
    }

    return dma_readb(DMA_IDX_REG, dmanr);
}

static __inline__ int get_dma_cmd_reg(unsigned int dmanr)
{
    if (dmanr >= MAX_DMA_CHANNELS) {
        printk("%s:invalid dmanr %d\n", __FUNCTION__, dmanr);
        return 0;
    }

    return dma_readb(DMA_CMD_REG, dmanr);
}

/* set burst length for a specific DMA channel */
static __inline__ void set_dma_burst_length(unsigned int dmanr, char burst)
{
	unsigned char val; 
	
	if (dmanr >= MAX_DMA_CHANNELS) 
		return;
	
	val = dma_readb(DMA_CMD_REG, dmanr);
	if (burst) /* DMA_BURST_LINE */
		val |= DMA_BURST_LINE;
	else /* DWORD burst */
		val &= ~DMA_BURST_LINE;
	
	dma_writeb(DMA_CMD_REG, dmanr, val);
}

#endif /* __ASM_MIPS_DMA_H */
