/******************************************************************************
 * @(#)dma.c : arch dma init function
 *
 * Copyright (c) 2002 Prolific Technology Inc. All Rights Reserved.
 *
 * This software is the confidential and proprietary information of Prolific
 * Technology, Inc. ("Confidential Information").  You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with Prolific.
 *
 * Modification History:
 *   #000 2005-12-27 jedy    create file
 *
 ******************************************************************************/

#include <linux/config.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/pci.h>

#include <asm/dma.h>
#include <asm/io.h>
#include <asm/mach/dma.h>
#include <asm/hardware.h>


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



typedef struct dma_mdt {
    unsigned    int saddr;      /* starting address */
    unsigned    int flag;       /* a entry flag */
} dma_mdt_t;

#define MDT_FLAG_DCNT_MASK      0x001FFFFF
#define MDT_FLAG_E              0x80000000
#define MDT_FLAG_V              0x40000000
#define MDT_FLAG_NEXT_MASK      0x1FE00000
#define MDT_FLAG_NXT_SHIFT      21
#define MDT_FLAG_VALID_SHIFT    30
#define MDT_FLAG_EMDT_SHIFT     31

#define MAX_DMA_MDT         4
#define MAX_DMA_MDT_COUNT   ((1 << 21) - 16)

struct arch_dma_t {
    int         lock;
    dma_mdt_t   *mde;
    dma_mdt_t   mde_default[MAX_DMA_MDT];
    dma_t       *dma;
} arch_dma_chan[MAX_DMA_CHANNELS];

static int pl_request_dma(dmach_t channel, dma_t *dma)
{
    struct arch_dma_t *arch_dma = &arch_dma_chan[channel];

    if (channel >= MAX_DMA_CHANNELS)
        return -EINVAL;

    if (xchg(&arch_dma->lock, 1) != 0)
        return -EBUSY;

    arch_dma->mde = NULL;       /* reset mde_base */
    return 0;
}

static void pl_free_dma(dmach_t channel, dma_t *dma)
{
    struct arch_dma_t *arch_dma = &arch_dma_chan[channel];

    if (channel >= MAX_DMA_CHANNELS) {
        printk(KERN_ERR "Trying to free DMA%d\n", channel);
        return;
    }

    if (xchg(&arch_dma->lock, 0) == 0) {
        printk(KERN_ERR "Trying to free a unrequest dma %d (%s)\n", channel, dma->device_id);
    }
}

static inline unsigned char dma_readb(unsigned int reg, unsigned int dmanr)
{
    return readb(PL_DMA_BASE + reg + sizeof(int) * dmanr);
}

static inline void dma_writeb(unsigned char value, unsigned int reg, unsigned int dmanr)
{
    writeb(value, PL_DMA_BASE + reg + sizeof(int) * dmanr);
}

static inline unsigned int dma_readl(unsigned int reg, unsigned int dmanr)
{
    return readl(PL_DMA_BASE + reg + sizeof(int) * dmanr);
}

static inline void dma_writel(unsigned int value, unsigned int reg, unsigned int dmanr)
{
    writel(value, PL_DMA_BASE + reg + sizeof(int) * dmanr);
}


static void pl_enable_dma(dmach_t channel, dma_t *dma)
{
    unsigned char val;
    struct arch_dma_t *arch_dma;
    int count;
    int i;
    unsigned int dcnt = 0;

    if (channel >= MAX_DMA_CHANNELS) {
        printk(KERN_ERR "Invalid dma channel %d\n", channel);
        return;
    }

    arch_dma = &arch_dma_chan[channel];

    if (dma->using_sg == 0) {
        if (arch_dma->mde == NULL) {
            arch_dma->mde = arch_dma->mde_default;
        }

        arch_dma->mde[0].saddr = (unsigned int)virt_to_bus(dma->buf.address);
        count = dma->buf.length;
        if (count > MAX_DMA_MDT * MAX_DMA_MDT_COUNT) {
            printk(KERN_ERR "Exceed max transfer size %d\n", count);
            return;
        }
        for (i = 0; i < MAX_DMA_MDT; i++) {
            if (i > 0) {
                arch_dma->mde[i].saddr = arch_dma->mde[i-1].saddr + dcnt;
            }
            if (count > MAX_DMA_MDT_COUNT)
                dcnt = MAX_DMA_MDT_COUNT;
            else
                dcnt = count;
            count -= dcnt;
            arch_dma->mde[i].flag = dcnt | ((i+1) << MDT_FLAG_NXT_SHIFT) | MDT_FLAG_V;
            if (count == 0)
                break;
        }

        arch_dma->mde[i].flag |= MDT_FLAG_E;
        dma_writeb(0, DMA_IDX_REG, channel);
        dma_writel((unsigned int) virt_to_bus(arch_dma->mde), DMA_MDT_REG, channel);
        pci_map_single(NULL, arch_dma->mde, MAX_DMA_MDT*sizeof(dma_mdt_t), PCI_DMA_TODEVICE);

    } else {
        if (dma->prolific_sg) {
            /* do nothing */
        } else {
            /* TBD */
            printk(KERN_ERR "Not implement yet!!!!");
            return;
        }
    }

    val = dma_readb(DMA_CMD_REG, channel);

    if (dma->dma_mode == DMA_MODE_READ)
        val |= DMA_MODE_UPSTREAM;
    else
        val &= ~DMA_MODE_UPSTREAM;

    dma_writeb(val | DMA_CHN_ENABLE, DMA_CMD_REG, channel);

}

static void pl_disable_dma(dmach_t channel, dma_t *dma)
{
    unsigned char val;

    if (channel >= MAX_DMA_CHANNELS) {
        printk(KERN_ERR "Invalid dma channel %d\n", channel);
        return;
    }

    val = dma_readb(DMA_CMD_REG, channel);
    dma_writeb(val & ~DMA_CHN_ENABLE, DMA_CMD_REG, channel);
}

static int pl_get_dma_residue(dmach_t channel, dma_t *dma)
{
    unsigned char val;

    if (channel >= MAX_DMA_CHANNELS) {
        printk(KERN_ERR "Invalid dma channel %d\n", channel);
        return -EINVAL;
    }

    val = dma_readb(DMA_CMD_REG, channel);
    return (val & DMA_CHN_ENABLE) ? 1 : 0;
}

static int pl_set_dma_speed(dmach_t channel, dma_t *dma, int cycle_ns)
{
    return 0;
}


/*
 * Prolific specifical mde functions
 */

void set_dma_mde_base(dmach_t channel, const char *mde_vaddr)
{
    struct arch_dma_t *arch_dma;
    dma_t *dma;

    if (channel >= MAX_DMA_CHANNELS) {
        printk(KERN_ERR "Invalid dma channel %d\n", channel);
        return;
    }

    if (((unsigned long)mde_vaddr) & 0x03) {
        printk(KERN_ERR "set channel %d mde, invalid mde_base address %p (must alignment 4 bytes)\n",
                channel, mde_vaddr);
        return;
    }

    arch_dma = &arch_dma_chan[channel];
    dma = arch_dma->dma;

    if (dma->active) {
        printk(KERN_ERR "dma%d: altering DMA mde while DMA active\n", channel);
    }

    dma->using_sg = 1;
    dma->prolific_sg = 1; /* using mde insead of sg */
    arch_dma->mde = (dma_mdt_t *)mde_vaddr;
    dma_writeb(0, DMA_IDX_REG, channel);
    dma_writel((unsigned int) virt_to_bus(&arch_dma->mde[0]), DMA_MDT_REG, channel);
}

void set_dma_mde_idx(dmach_t channel, unsigned int mde_idx)
{
    if (channel >= MAX_DMA_CHANNELS) {
        printk(KERN_ERR "Invalid dma channel %d\n", channel);
        return;
    }
    if (mde_idx > 255) {
        printk(KERN_ERR "Set dma channel %d invalide mde_idx %d\n" , channel, mde_idx);
        return;
    }

    dma_writeb(mde_idx, DMA_IDX_REG, channel);

}


void set_dma_burst_length(dmach_t channel, char burst)
{
    unsigned char val;

    if (channel >= MAX_DMA_CHANNELS) {
        printk(KERN_ERR "Invalid dma channel %d\n", channel);
        return;
    }

    val = dma_readb(DMA_CMD_REG, channel);
    if (burst) /* DMA_BURST_LINE */
        val |= DMA_BURST_LINE;
    else /* DWORD burst */
        val &= ~DMA_BURST_LINE;

    dma_writeb(val, DMA_CMD_REG, channel);

}


struct dma_ops pl_dma_ops = {
    request:    pl_request_dma,
    free:       pl_free_dma,
    enable:     pl_enable_dma,
    disable:    pl_disable_dma,
    residue:    pl_get_dma_residue,
    setspeed:   pl_set_dma_speed,
    type:       "",
};

void __init arch_dma_init(dma_t *dma)
{
    int i;
    for (i = 0; i < MAX_DMA_CHANNELS; i++, dma++) {
        dma->d_ops = &pl_dma_ops;
        memset(&arch_dma_chan[i], 0, sizeof(struct arch_dma_t));
        arch_dma_chan[i].dma = dma;
    }
}

EXPORT_SYMBOL(set_dma_mde_base);
EXPORT_SYMBOL(set_dma_mde_idx);
EXPORT_SYMBOL(set_dma_burst_length);
