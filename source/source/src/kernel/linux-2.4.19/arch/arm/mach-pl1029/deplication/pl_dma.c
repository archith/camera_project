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

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <asm/arch/pl_dma.h>
#include <asm/system.h>

//#define ALIGN4
//hikari
#ifndef ALIGN4
#define ALIGN4(x)   (((x + 3)) & -4)
#endif

/* A note on resource allocation:
 *
 * All drivers needing DMA channels, should allocate and release them
 * through the public routines `request_dma()' and `free_dma()'.
 *
 * In order to avoid problems, all processes should allocate resources in
 * the same sequence and release them in the reverse order.
 *
 * So, when allocating DMAs and IRQs, first allocate the IRQ, then the DMA.
 * When releasing them, first release the DMA, then release the IRQ.
 * If you don't, you may cause allocation requests to fail unnecessarily.
 * This doesn't really matter now, but it will once we get real semaphores
 * in the kernel.
 */


spinlock_t dma_spin_lock = SPIN_LOCK_UNLOCKED;

/* Channel n is busy iff dma_chan_busy[n].lock != 0.
 */


#define DMA_MDE_DCNT_MASK   0x001FFFFF
#define DMA_MDE_E           0x80000000
#define DMA_MDE_V           0x40000000
#define DMA_MDE_NEXT_MASK   0x1FE00000
#define DMA_MDE_NEXT_SHIFT  20


static struct dma_chan dma_chan_busy[MAX_DMA_CHANNELS];

static char *get_mdt(unsigned int dmanr)
{
    static char buf[256];
    int i, len = 0;
    unsigned int mde;

    len += sprintf(buf, "\n    MDT=%08lx ",virt_to_bus(&dma_chan_busy[dmanr].mdt[0]));
    if (dma_chan_busy[dmanr].mdt == NULL)
        return buf;

    for (i = 0; i < MAX_DMA_MDE; i++) {
        mde = dma_chan_busy[dmanr].mdt[i].mde;
        len += sprintf(buf + len, "MDE[%d]=(%08x, %d%d %3d %6d) ", 
            i, dma_chan_busy[dmanr].mdt[i].saddr, 
            mde >> 31, (mde >> 30) & 0x01,
            (mde >> 21) & 0xFF, (mde & ((1 << 21) - 1))
            );
    }
    return buf;
}

int get_dma_list(char *buf)
{
	int i, len = 0;

	for (i = 0 ; i < MAX_DMA_CHANNELS ; i++) {
		if (dma_chan_busy[i].lock) {
		    len += sprintf(buf+len, "%2d: %s (idx=%d cmd=0x%x) %s\n",
				   i,  dma_chan_busy[i].device_id, 
                   ((dma_chan_busy[i].device_id) ? dma_readb(DMA_IDX_REG, i) : 0), 
                   ((dma_chan_busy[i].device_id) ? dma_readb(DMA_CMD_REG, i) : 0),
                    get_mdt(i));
		}
	}
	return len;
} /* get_dma_list */


int request_dma(unsigned int dmanr, const char * device_id)
{
	if (dmanr >= MAX_DMA_CHANNELS)
		return -EINVAL;

	if (xchg(&dma_chan_busy[dmanr].lock, 1) != 0)
		return -EBUSY;

    dma_chan_busy[dmanr].mdt = NULL;            /* reset mde_base */
	dma_chan_busy[dmanr].device_id = device_id;
	/* old flag was 0, now contains 1 to indicate busy */
	return 0;
} /* request_dma */


void free_dma(unsigned int dmanr)
{
	if (dmanr >= MAX_DMA_CHANNELS) {
		printk("Trying to free DMA%d\n", dmanr);
		return;
	}

	if (xchg(&dma_chan_busy[dmanr].lock, 0) == 0) {
		printk("Trying to free free DMA%d\n", dmanr);
		return;
	}	

} /* free_dma */


/* 
 * Set transfer address & page bits for specific DMA channel.
 * the input address must be word aligned
 */
void set_dma_addr(unsigned int dmanr, unsigned int a)
{
    //printk("pl_dma.c,set_dma_addr(dmanr=%d)\n",dmanr);
    if (dmanr >= MAX_DMA_CHANNELS) {
        printk("%s:invalid dmanr %d\n", __FUNCTION__, dmanr);
        return;
    }
    if (dma_chan_busy[dmanr].mdt == NULL) 
        dma_chan_busy[dmanr].mdt = dma_chan_busy[dmanr].mdt_default;

/* Added by Stefano(2005/01/31)
 * fix debug for SD driver
 * The paramete a is physical address
 * Not need to use virt_to_bus to transfer physcial address
 */
#if 1
		dma_chan_busy[dmanr].mdt[0].saddr = (unsigned int)((void *)ALIGN4(a));
#else    
    dma_chan_busy[dmanr].mdt[0].saddr = (unsigned int)virt_to_bus((void *)ALIGN4(a));
#endif    
    //printk("set_dma_addr(1)dma_chan_busy[dmanr].mdt[0].saddr=%x\n",dma_chan_busy[dmanr].mdt[0].saddr);
}


/* Set transfer size 
 * The max transfer size is 2M * MAX_DMA_MDE_COUNT
 */

#define MDE_NXT_SHIFT       21
#define MDE_VALID_SHIFT     30
#define MDE_EMDE_SHIFT      31

void set_dma_count(unsigned int dmanr, unsigned int count)
{
    int i;
    unsigned int dcnt = 0;

    if (dmanr >= MAX_DMA_CHANNELS) {
        printk("%s:invalid dmanr %d\n", __FUNCTION__, dmanr);
        return;
    } 

    if (dma_chan_busy[dmanr].mdt == NULL) {
        printk("%s:wrong call sequence, should invoke set_dma_addr before set_dma_count\n", __FUNCTION__);
        return;
    }

    count = ALIGN4(count);

    if (count > MAX_DMA_MDE * MAX_DMA_MDE_COUNT) {
        printk("%s:exceed max transfer size %d\n", __FUNCTION__, count);
        return;
    }

    for (i = 0; i < MAX_DMA_MDE; i++) {

        if (i > 0) {
            dma_chan_busy[dmanr].mdt[i].saddr = 
                dma_chan_busy[dmanr].mdt[i-1].saddr + dcnt;
        }

        if (count > MAX_DMA_MDE_COUNT) 
            dcnt = MAX_DMA_MDE_COUNT;
        else
            dcnt = count;
        count -= dcnt;
        
        dma_chan_busy[dmanr].mdt[i].mde = dcnt | 
                                         ((i + 1) << MDE_NXT_SHIFT) |
                                         (1 << MDE_VALID_SHIFT);
        if (count == 0)
            break;
    }

    dma_chan_busy[dmanr].mdt[i].mde |= (1 << MDE_EMDE_SHIFT);
    dma_writeb(DMA_IDX_REG, dmanr, 0);   /* reset mde index */
    dma_writel(DMA_MDT_REG, dmanr, (unsigned int) virt_to_bus(dma_chan_busy[dmanr].mdt));
}

void set_dma_mde_base(unsigned int dmanr, const char *mde_vaddr)
{
    if (dmanr >= MAX_DMA_CHANNELS) {
        printk("%s:invalid dmanr %d\n", __FUNCTION__, dmanr);
        return;
    }
    if (((unsigned long) mde_vaddr) & 0x04) {
        printk("%s:dmanr %d invalide mde_base address %p\n" ,__FUNCTION__, dmanr, mde_vaddr);
        return;
    }

    dma_chan_busy[dmanr].mdt = (dma_mdt_t *)mde_vaddr;
    dma_writeb(DMA_IDX_REG, dmanr, 0);
    dma_writel(DMA_MDT_REG, dmanr, (unsigned int) virt_to_bus(&dma_chan_busy[dmanr].mdt[0]));
}


#ifdef CONFIG_CF_DMA
/*
 * Set transfer address & page bits for specific DMA channel.
 * the input address must be word aligned
 */
void cf_set_dma_addr(unsigned int dmanr, unsigned int addr, unsigned int mdt_index)
{
    printk("cf_set_dma_addr ! \n");
    if (dmanr >= MAX_DMA_CHANNELS) {
        printk("%s:invalid dmanr %d\n", __FUNCTION__, dmanr);
        return;
    }

    dma_chan_busy[dmanr].mdt[mdt_index].saddr = (unsigned int)virt_to_bus((void *)ALIGN4(addr));
}
/* Set transfer size
 * The max transfer size is 2M * MAX_DMA_MDE_COUNT
 */
void cf_set_dma_count(unsigned int dmanr, unsigned int count, unsigned int mdt_index)
{
    int i;
    unsigned int dcnt = 0;
    printk("cf_set_dma_count ! \n");
    if (dmanr >= MAX_DMA_CHANNELS) {
        printk("%s:invalid dmanr %d\n", __FUNCTION__, dmanr);
        return;
    }

    count = ALIGN4(count);

    if (count > MAX_DMA_MDE * MAX_DMA_MDE_COUNT) {
        printk("%s:exceed max transfer size %d\n", __FUNCTION__, count);
        return;
    }

    for (i = 0; i < MAX_DMA_MDE; i++) {

        if (i > 0) {
            dma_chan_busy[dmanr].mdt[i].saddr =
                dma_chan_busy[dmanr].mdt[i-1].saddr + dcnt;
        }

        if (count > MAX_DMA_MDE_COUNT)
            dcnt = MAX_DMA_MDE_COUNT;
        else
            dcnt = count;
        count -= dcnt;

        dma_chan_busy[dmanr].mdt[i].mde = dcnt |
                                         ((i + 1) << MDE_NXT_SHIFT) |
                                         (1 << MDE_VALID_SHIFT);
        if (count == 0)
            break;
    }

    dma_chan_busy[dmanr].mdt[i].mde |= (1 << MDE_EMDE_SHIFT);
    dma_writeb(DMA_IDX_REG, dmanr, 0);   /* reset mde index */
    
    //modified by chun-wen 2003-09-05 
    //because dma_writel() is to be (unsigned int) , and 
    // dma_writew() is to be (unsigned short) . 
    //dma_writew(DMA_MDT_REG, dmanr, (unsigned int) virt_to_bus(&dma_chan_busy[dmanr].mdt[0]));
    dma_writel(DMA_MDT_REG, dmanr, (unsigned int) virt_to_bus(&dma_chan_busy[dmanr].mdt[0]));
}
#endif
