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
#define NORMTD_VERSION "0.8.0"
 
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/dma.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/semaphore.h>

#include <linux/mtd/mtd.h> 
#include <linux/mtd/partitions.h>

#ifndef CONFIG_ARCH_PL1029
#error "Prolific PL-1029 platform is missing?!"
#endif

#ifndef rNOR_BASE
#error "No proper base register defined for PL-1029 NOR flash"
#endif

/* Prolific NOR flash controller related operations */
/* AMD PPI only for now */

/* NOR flash controller related */

/* register definitions */
#define PIO_DATA_REG	(rNOR_BASE + 0x00)
#define CONFIG_REG	(rNOR_BASE + 0x04)
#define COM_REG		(rNOR_BASE + 0x08)
#define INTR_S_REG	(rNOR_BASE + 0x0c)
#define NOR_ADDR_REG	(rNOR_BASE + 0x10)
#define CTRL_ST_REG	(rNOR_BASE + 0x14)
#define INTR_M_REG	(rNOR_BASE + 0x18)
#define SOFT_RST_REG	(rNOR_BASE + 0x1c)
#define DMA_S_REG	(rNOR_BASE + 0x20)
#define TIMER_REG	(rNOR_BASE + 0x24)
#define PAD_REG		(rNOR_BASE + 0x28)
#define PPI_PRO_CNT_REG	(rNOR_BASE + 0x2c)
#define ADDR_CFG1	(rNOR_BASE + 0x34)
#define ADDR_CFG2	(rNOR_BASE + 0x38)
#define ADDR_CFG3	(rNOR_BASE + 0x3c)
#define ADDR_CFG4	(rNOR_BASE + 0x40)
#define ADDR_CFG5	(rNOR_BASE + 0x44)
#define DATA_CFG1	(rNOR_BASE + 0x48)
#define DATA_CFG2	(rNOR_BASE + 0x4c)
#define DATA_CFG3	(rNOR_BASE + 0x50)
#define DATA_CFG4	(rNOR_BASE + 0x54)
#define DATA_CFG5	(rNOR_BASE + 0x58)

/* interrupt */
#define NOR_TIMEOUT_INTR_MASK   (1<<3)
#define NOR_CMD_INTR_MASK       (1<<2)
#define NOR_DMA_INTR_MASK       (1<<0)

#define NOR_INTR_MASKALL	(NOR_TIMEOUT_INTR_MASK|NOR_CMD_INTR_MASK|NOR_DMA_INTR_MASK)
#define NOR_INTR_SET_MASK(m)	\
do {				\
	unsigned char mask;	\
	mask = 0x00;		\
	mask |= m;		\
	writeb(mask, INTR_M_REG);\
} while(0)

static int pl_nor_polling_intr(int, int);

#define MAX_NOR_SECTOR 1024
#define NOR_TYPE_NUM 3
static const char *nor_type[NOR_TYPE_NUM] = { "spi", "ppi-amd", "ppi-intel" };
static int norflash_tag = 0;
static struct tag_norinfo nor_geometry;
typedef struct {
	unsigned long start_addr;
	unsigned int size;
} nor_sector_t;
static nor_sector_t nor_sector_map[MAX_NOR_SECTOR];
static int total_sectors = 0;
static int total_size = 0; /* in bytes */
static int num_erase_regions = 0;

/*
 * Actually 4MB-1, but we want easy implementation for 16-byte aligned, so...
 */
#define MAX_DMA_CNT	0x3fff00
#define DMA_START	1
#define DMA_END		2

typedef struct {
	int (*init)(void);
	int (*reset)(void);
	int (*read_id)(u8 *);
	int (*cmd_erase)(unsigned long, int);
	int (*cmd_write)(unsigned long, int, int);
	int (*cmd_read)(unsigned long, int, int);
	int (*pio_read)(unsigned long, char *, int);
	int (*pio_write)(unsigned long, char *, int);
} pl_nor_ops_t;
static pl_nor_ops_t *nor_ops = NULL;

/* erase and write commnads interval counts from h/w engineer */
static unsigned long ppi_write_wait_count = 0x500;
static unsigned long ppi_erase_wait_count = 0x1050000;

static u8 intr_status;
#if 0
static char *dma_buf = NULL;
static int dma_buf_size = 0;
#endif

#define MDE_NXT_SHIFT           21
#define MDE_VALID_SHIFT         30
#define MDE_EMDE_SHIFT          31

typedef struct {
	unsigned long saddr;
	unsigned long mde;
} dma_mde_t;

#ifndef MAX_SECTORS
#define MAX_SECTORS 128
#endif
#define MDE_MAX_ENTRY   MAX_SECTORS
static dma_mde_t mdt[MDE_MAX_ENTRY] __attribute__ ((aligned (8)));
static struct iovec iovec_tbl[MDE_MAX_ENTRY+1];

/*
 * taginfo passed by Proboot, it's the nor flash mapping table 
 * We rely totally on this info. Without this, we won't initialize.
 */
#define MAX_SECTOR_SET	16
struct tag_norinfo {
        u32	type;
        struct tag_sectinfo {
                u16	size;
                u16	quantum;
        } sectinfo[MAX_SECTOR_SET];
};
#define ATAG_NORINFO 0x10290101

static int __init pl1029_parse_tag_norinfo(const struct tag *tag)
{
        memcpy(&nor_geometry, &(tag->u), sizeof(nor_geometry));
        norflash_tag = 1;
        
        return 0;
}
__tagtable(ATAG_NORINFO, pl1029_parse_tag_norinfo);

/* Prolific DMA controller related functions */

static int pl_setup_dma(int mode, struct iovec *vecs, int count)
{
        int i, total_len;

        set_dma_mode(DMA_PL_NOR, mode);
        set_dma_mde_idx(DMA_PL_NOR, 0);
        
        /* setup MDE table */
        total_len = 0;
        for ( i=0; i<count; i++ ) {
                mdt[i].saddr = virt_to_bus(vecs[i].iov_base);
                mdt[i].mde = vecs[i].iov_len |
                             ((i + 1) << MDE_NXT_SHIFT) |
                             (1 << MDE_VALID_SHIFT);
                total_len += vecs[i].iov_len;
                pci_map_single(NULL, vecs[i].iov_base, vecs[i].iov_len,
                               mode == DMA_MODE_READ ? PCI_DMA_FROMDEVICE : 
                                                       PCI_DMA_TODEVICE);
        }
        mdt[count-1].mde |= (1 << MDE_EMDE_SHIFT);

        pci_map_single(NULL, mdt, sizeof(dma_mde_t)*(MDE_MAX_ENTRY+1), 
                       PCI_DMA_TODEVICE);

        return total_len;
}

static void pl_unmap_dma(struct iovec *vecs, int count, int mode)
{
        int i;
        
        for ( i=0; i<count; i++ ) {
                pci_unmap_single(NULL, virt_to_bus(vecs[i].iov_base),
                                 vecs[i].iov_len, 
                                 mode == DMA_MODE_READ ? PCI_DMA_FROMDEVICE : 
                                                         PCI_DMA_TODEVICE);
        }
        pci_unmap_single(NULL, virt_to_bus(mdt), 
                         sizeof(dma_mde_t)*(MDE_MAX_ENTRY+1), PCI_DMA_TODEVICE);
}

/* nor flash controller semaphore */
static DECLARE_MUTEX(ctl_sem);
static DECLARE_WAIT_QUEUE_HEAD(wq_intr);

/* PPI AMD related command functions */

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
	writel(0xAA,  PIO_DATA_REG);
	writel(0x555, NOR_ADDR_REG);
	writel(0x55,  PIO_DATA_REG);
	writel(0xAAA, NOR_ADDR_REG);
	writel(0x90,  PIO_DATA_REG);
	writel(0x02,  NOR_ADDR_REG);
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

int ppi_amd_pio_read(unsigned long addr, char *buf, int size)
{
	char *p;
	int val, remains, count, rc;
	
	rc = 0;
	remains = size;
	p = buf;

	writel(0x30, COM_REG);
	while ( remains > 0 ) {
		writel(addr, NOR_ADDR_REG);
		if ( remains >= 4 ) {
			writel(readl(PIO_DATA_REG), p);
			count = 4;
		} else {
			val = readl(PIO_DATA_REG);
			memcpy(p, (char *)&val, remains);
			count = remains;
		}
		addr += count;
		p += count;
		remains -= count;
	}
	writel(0x00, COM_REG);
	
	return rc;
}

int ppi_amd_pio_write(unsigned long addr, char *buf, int size)
{
	char *p;
	int val, remains, count, rc;

	writel(ppi_write_wait_count, PPI_PRO_CNT_REG);
	writel(0x34,  COM_REG);
	writel(0xAAA, ADDR_CFG1);
	writel(0xAA,  DATA_CFG1);
	writel(0x555, ADDR_CFG2);
	writel(0x55,  DATA_CFG2);
	writel(0xAAA, ADDR_CFG3);
	writel(0xA0,  DATA_CFG3);

	rc = 0;
	remains = size;
	p = buf;
	
	while ( remains > 0 ) {
		writel(addr, NOR_ADDR_REG);
		if ( remains >= 4 ) {
			writel(readl(p), PIO_DATA_REG);
			count = 4;
		} else {
			memset((char *)&val, 0xff, 4);
			memcpy((char *)&val, p, remains);
			count = remains;
		}
		rc = pl_nor_polling_intr(NOR_CMD_INTR_MASK, 0);
		if ( rc == 0 )
			break;
		addr += count;
		p += count;
		remains -= count;
	}
	writel(0x00, COM_REG);
	
	return rc;
}

pl_nor_ops_t ppi_amd = {
	.init =		ppi_amd_init,
	.reset = 	ppi_amd_reset,
	.read_id = 	ppi_amd_read_id,
	.cmd_erase = 	ppi_amd_cmd_erase,
	.cmd_write =	ppi_amd_cmd_write,
	.cmd_read =	ppi_amd_cmd_read,
	.pio_read = 	ppi_amd_pio_read,
	.pio_write =	ppi_amd_pio_write,
};

/* NOR flash controller related functions */

/* for polling the controller interrupt status registers */
static int pl_nor_polling_intr(int intr_bit, int expected)
{
	unsigned long start_j = jiffies;
	int rc;
	
	rc = 0;	
	while ( (start_j + HZ) >= jiffies ) { /* max. polling time 1 sec. */
		intr_status = readb(INTR_S_REG);
		if ( (intr_status & 0x08) == 0 ) /* controller time-out */
			break;
		if ( (intr_status & intr_bit) == expected ) {
			rc = 1;
			break;
		}
	}
	
	return rc;
}

static void pl_nor_do_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
        intr_status = readb(INTR_S_REG);
#if 0
        printk("NOR INTR: intr_status = 0x%02x\n", intr_status);
#endif        
        wake_up(&wq_intr);

        return;
}

static void __init pl_nor_mtd_banner(void)
{
        printk("PL-1029 NOR flash driver for MTD, version %s\n", NORMTD_VERSION);
}

static int __init pl_nor_init_controller(void)
{
/* 
 * no need to initialize the controller for now. 
 * Proboot has already done it. So, we only show some regs.
 */
#if 0
        printk("config = 0x%08x\n", readb(CONFIG_REG));
#endif        
        return 0;
}

static int __init pl_nor_generate_map(void)
{
        int i, j, size, sec_idx;
        u8 id;
        unsigned long addr;

        i = nor_geometry.type;        
        printk("NOR flash type: %s ", i<NOR_TYPE_NUM?nor_type[i]:"unknown");
        sec_idx = 0;
        addr = 0;
        for ( i=0; i<MAX_SECTOR_SET; i++ ) {
                size = nor_geometry.sectinfo[i].size;
                if ( size == 0 )
                        break;
                printk("%dx%d ", size, nor_geometry.sectinfo[i].quantum);
                num_erase_regions ++;
#if 0
                if ( (size<<10) > dma_buf_size )
                        dma_buf_size = (size<<10);
#endif                        
                for ( j=0; j<nor_geometry.sectinfo[i].quantum; j++ ) {
                        nor_sector_map[sec_idx].start_addr = addr;
                        nor_sector_map[sec_idx].size = (size<<10);
                        total_size += (size<<10);
                        addr += nor_sector_map[sec_idx].size;
                        sec_idx ++;
                        if ( sec_idx == MAX_NOR_SECTOR ) {
                                printk("NOR flash sector > %d, ignore the rest\n", MAX_NOR_SECTOR);
                                break;
                        }
                }                                 
        }
        total_sectors = sec_idx;
        printk("\n");
#if 0
for ( i=0; i<total_sectors; i++ ) {
        printk("sector %d: %lu, %u\n", i, nor_sector_map[i].start_addr, nor_sector_map[i].size);
}
#endif        
        switch ( nor_geometry.type ) {
                case 0: /* spi */
                        printk("NOR: spi not supported now\n");
                        return -1;
                case 1: /* ppi amd */
                        nor_ops = &ppi_amd;
                        break;
                case 2: /* ppi intel */
                        printk("NOR: ppi intel not supported now\n");
                        return -1;
                default:
                        printk("NOR: unknown flash type\n");
                        return -1;
        }
        nor_ops->init();
        nor_ops->read_id(&id);
        printk("NOR flash id = 0x%02x\n", id);

        return 0;
}

static int __init pl_nor_init_dma(void)
{
#if 1
	int rc;
#else
/* We won't need this for now. Use the buffer passed from MTD */
        int rc, size, order;
        int page_cnt[10] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 };

        if ( dma_buf_size == 0 ) {
                printk("NOR: dma_buf_size == 0? failed to initialize\n");
                return -1;
        }
        size = (dma_buf_size >> 12);
        for ( order=0; order<10; order++ ) {
                if ( page_cnt[order] == size )
                        break;
        }
        if ( order >= 10 ) {
                printk("NOR: page count = %d, not a happy value for me, failed init.\n", size);
                return -1;
        }
        /* Use __get_free_pages to get pages to be used in DMA transfer.
         * Note: because of page nature, it's guaranteed to be 16-byte
         * aligned(4K page aligned exactly), which is our controller's
         * requirement for DMA.
         */
        dma_buf = (char *)__get_free_pages(__GFP_DMA, order);
        if ( !dma_buf ) {
                printk("NOR: cannot get %d pages for DMA, failed init.\n", size);
                return -1;
        }
#endif
         
#if 0
        printk("NOR: allocate %d pages for DMA buffer\n", size);
#endif
        memset(iovec_tbl, 0x00, sizeof(struct iovec)*(MDE_MAX_ENTRY+1));
        rc = request_dma(DMA_PL_NOR, "NOR");
        if ( rc < 0 )
                return rc;
                
        set_dma_mde_base(DMA_PL_NOR, (const char *)mdt);
        set_dma_mode(DMA_PL_NOR, DMA_MODE_READ);
        set_dma_burst_length(DMA_PL_NOR, DMA_BURST_LINE);

        return 0;
}

static int __init pl_nor_init_irq(void)
{
        int rc;
        
        intr_status = readb(INTR_S_REG); /* in case there are still pending intr */
        
        rc = request_irq(IRQ_PL_NOR, pl_nor_do_interrupt, 0, "nor", NULL);
        if ( rc ) {
                printk("NOR: cannot register IRQ%d for block driver, rc=%d\n",
                       IRQ_PL_NOR, rc);
                return -1;
        } else {
                printk("nor interrupt %d registered\n", IRQ_PL_NOR);
        }

        /* mask three interrupts */
        NOR_INTR_SET_MASK(NOR_INTR_MASKALL);
        
        return rc;
}

static int __init pl_nor_init(void)
{
	pl_nor_mtd_banner();
	if ( !norflash_tag ) {
		printk("No NOR flash tag passed by Proboot, failed to initialize\n");
		return -1;
	}
	pl_nor_init_controller();
	pl_nor_generate_map();
	
	if ( pl_nor_init_dma()<0 )
		return -1;
		
	if ( pl_nor_init_irq()<0 )
		return -1;
		
	return 0;
}

static int pl_nor_erase_one_block(unsigned long addr)
{
        int rc = 0;

        NOR_INTR_SET_MASK(NOR_DMA_INTR_MASK);
        intr_status = 0x00;
        nor_ops->cmd_erase(addr, DMA_START);
        sleep_on(&wq_intr);
        if ( (intr_status & 0x04) != 0 ) {
                printk("nor erase sector %lu failed?!\n", addr);
        }
        rc = nor_ops->cmd_erase(addr, DMA_END);
        if ( rc < 0 ) {
                printk("nor erase data polling error?!\n");
        }
        NOR_INTR_SET_MASK(NOR_INTR_MASKALL);
        
        return rc;
}

static int pl_nor_map_sector(unsigned long addr, nor_sector_t *sector)
{
        int i;
        int rc = -EINVAL;
        
        for ( i=0; i<total_sectors; i++ ) {
                if ( (addr >= nor_sector_map[i].start_addr) &&
                     (addr < nor_sector_map[i].start_addr + 
                             nor_sector_map[i].size )          
                             ) {
                        sector->start_addr = nor_sector_map[i].start_addr;
                        sector->size = nor_sector_map[i].size;
                        rc = 0;
                        break;
                }
        }
        
        return rc;
}

/* MTD related functions */

static struct mtd_info pl_nor_mtd;
static char pl_nor_mtd_name[] = "plnormtd";

static int pl_nor_flash_read(struct mtd_info *mtd, loff_t from, size_t len,
			     size_t *retlen, u_char *buf)
{
	int rc, pio_len = 0, this_cnt;
	u_char *buf_now;
	loff_t addr;
	
#if 0
printk("NOR read from %lld len = %d to buf %p\n", from, len, buf);
#endif

	rc = -EIO;
		
	if ( from+len > mtd->size )
		return -EINVAL;

        if ( down_interruptible(&ctl_sem) ) {
                return -ERESTARTSYS;
        }
        
        addr = from;
        buf_now = buf;
        *retlen = 0;
        if ( (unsigned long)buf_now & 0x0f ) {
        	pio_len = (unsigned long)buf_now & 0x0f;
#if 0
printk("NOR pio read: %lld\n", from);
#endif        	
        	nor_ops->pio_read(from, buf_now, pio_len);
        	buf_now += pio_len;
        	*retlen = pio_len;
        	len -= pio_len;
        	addr += pio_len;
        }

        rc = 0;
        while ( len > 0 ) {
        	this_cnt = len > MAX_DMA_CNT ? MAX_DMA_CNT : len;
                iovec_tbl[0].iov_base = buf_now;
                iovec_tbl[0].iov_len = this_cnt;
                if ( pl_setup_dma(DMA_MODE_READ, iovec_tbl, 1) != this_cnt ) {
                        printk("Cannot setup PL DMA channel for read\n");
                        rc = -EFAULT;
                        goto out;
                }
        
                intr_status = 0x00;
                NOR_INTR_SET_MASK(NOR_CMD_INTR_MASK);
                enable_dma(DMA_PL_NOR);
                nor_ops->cmd_read(addr, this_cnt, DMA_START);
        
                sleep_on(&wq_intr);
                
                if ( !((intr_status & 0x01) == 0 && get_dma_residue(DMA_PL_NOR)==0) ) {
                        printk("nor mtd dma read error?!\n");
                        rc = -EIO;
                }
                NOR_INTR_SET_MASK(NOR_INTR_MASKALL);
                disable_dma(DMA_PL_NOR);
                pl_unmap_dma(iovec_tbl, 1, DMA_MODE_READ);
                nor_ops->cmd_read(addr, this_cnt, DMA_END);

                buf_now += this_cnt;
                len -= this_cnt;
                addr += this_cnt;
                *retlen += this_cnt;
        }
out:
        up(&ctl_sem);
        
        return rc;
}

static int pl_nor_flash_write(struct mtd_info *mtd, loff_t to, size_t len,
			      size_t *retlen, const u_char *buf)
{
	int rc, pio_len = 0, this_cnt;
	u_char *buf_now;
	loff_t addr;
	
#if 0
printk("NOR write to %lld len = %d from buf %p\n", to, len, buf);
#endif	
	rc = -EIO;
	
	if ( to+len > mtd->size )
		return -EINVAL;
	
        if ( down_interruptible(&ctl_sem) )
                return -ERESTARTSYS;

	addr = to;
	buf_now = (u_char *)buf;
	*retlen = 0;
	if ( (unsigned long)buf_now & 0x0f ) {
		pio_len = (unsigned long)buf_now & 0x0f;
#if 0
printk("NOR pio write: %lld\n", to);
#endif
		nor_ops->pio_write(to, buf_now, pio_len);
		buf_now += pio_len;
		*retlen = pio_len;
		len -= pio_len;
		addr += pio_len;
	}

	rc = 0;	
        while ( len > 0 ) {
        	this_cnt = len > MAX_DMA_CNT ? MAX_DMA_CNT : len;
        	iovec_tbl[0].iov_base = buf_now;
                iovec_tbl[0].iov_len = this_cnt;
                if ( pl_setup_dma(DMA_MODE_WRITE, iovec_tbl, 1) != this_cnt ) {
                        printk("Cannot setup PL DMA channel for write\n");
                        rc = -EFAULT;
                        goto out;
                }
        
                intr_status = 0x00;
                NOR_INTR_SET_MASK(NOR_CMD_INTR_MASK);
                enable_dma(DMA_PL_NOR);
                nor_ops->cmd_write(addr, this_cnt, DMA_START);
        
                sleep_on(&wq_intr);
        
                if ( !((intr_status & 0x01) == 0 && get_dma_residue(DMA_PL_NOR)==0) ) {
                        printk("nor mtd dma write error?!\n");
                        rc = -EIO;
                }
                NOR_INTR_SET_MASK(NOR_INTR_MASKALL);
                disable_dma(DMA_PL_NOR);
                pl_unmap_dma(iovec_tbl, 1, DMA_MODE_WRITE);
                nor_ops->cmd_write(addr, this_cnt, DMA_END);
                
                buf_now += this_cnt;
                len -= this_cnt;
                addr += this_cnt;
                *retlen += this_cnt;
        }
out:
        up(&ctl_sem);
        
        return rc;

}

static int pl_nor_flash_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	int len;
	unsigned long addr;
	int rc = -EINVAL;
#if 0
printk("nor mtd erase, addr = %u len = %u\n", instr->addr, instr->len);
#endif

	instr->state = MTD_ERASE_FAILED;

	if ( instr->addr > mtd->size ) 
		return -EINVAL;
	if ( instr->addr + instr->len > mtd->size )
		return -EINVAL;
#if 0
	/* 
	 * Do we need to check if the erase starting address and length
	 * are erase-block aligned? 
	 */
#endif		

	len = instr->len;
	addr = instr->addr;
	
	if ( down_interruptible(&ctl_sem) )
		return -ERESTARTSYS;
	
	while ( len > 0 ) {
		nor_sector_t sector;
		
		if ( pl_nor_map_sector(addr, &sector) < 0 ) {
			printk("nor addr %lu cannot be mapped to sector\n", addr);
			goto probably_erase_failed;
		}
#if 0
printk("nor mtd erase %lu block\n", sector.start_addr);
#endif
		if ( pl_nor_erase_one_block(sector.start_addr) < 0 ) {
			printk("nor erase block addr %lu failed\n", sector.start_addr);
			goto probably_erase_failed;
		}
		len -= sector.size;
		addr += sector.size;
	}
	
	rc = 0;
	instr->state = MTD_ERASE_DONE;
	if ( instr->callback )
		instr->callback(instr);
	
probably_erase_failed:	
	up(&ctl_sem);
		
	return rc;
}

static struct mtd_partition pl_nor_partitions[] = {
	{
		name: 		"bootloader",
		size: 		0x10000,	/* 64K */
		offset: 	0,
		mask_flags:	MTD_WRITEABLE, /* read-only */
	}, {
		name:		"MAC",
		size:		0x10000, /* 64K */
		offset:		0x10000, /* starts at 0x10000 */
	}, {
		name:		"SQUASHFS",
		size:		0x6C0000, /* 108*64K = 6912K */
		offset:		0x140000, /* starts at 0x140000 */
                mask_flags:     MTD_WRITEABLE,  /* force read-only */
	}, {
		name:		"CONFIG",
		size:		0x10000, /* 64K */
		offset:		0x20000, /* starts at 0x20000 */

	}, {
		name:		"KFS",
		size:		0x790000,
		offset:		0x70000, /* starts at 0x70000 */
	}, { /* this is overlapped with kernel and rootfs above */
		name:		"ALL",
		size:		0x800000,
		offset:		0x0,
	}, {
		name:		"LOGO Image",
		size:		0x10000,/* 64K */
		offset:		0x30000, /* starts at 0x30000 */
	}, {
		name:		"HTTPS CA",
		size:		0x10000, /* 64K */
		offset:		0x40000, /* starts at 0x40000 */
	}, {
		name:		"ROOT CA",
		size:		0x10000,	/* 64K */
		offset:		0x50000, /* starts at 0x50000 */
	}, {
		name:		"User CA",
		size:		0x10000,	/* 64K */
		offset:		0x60000, /* starts at 0x60000 */
	}
};

static int initialized = 0;

static void cleanup_mtd_struct(void)
{
	if ( pl_nor_mtd.eraseregions != NULL )
		kfree(pl_nor_mtd.eraseregions);
	memset(&pl_nor_mtd, 0x00, sizeof(struct mtd_info));
}

int __init pl_nor_mtd_init(void)
{
	int i, rc;
	
	if ( initialized ) {
		printk("NOR MTD already initialized?");
		return -1;
	}
	
	rc = pl_nor_init();
	if ( rc < 0 )
		return rc;
		
	memset(&pl_nor_mtd, 0x00, sizeof(struct mtd_info));
	pl_nor_mtd.type = MTD_NORFLASH;
	pl_nor_mtd.flags = MTD_CAP_NORFLASH;
	pl_nor_mtd.size = total_size;
	pl_nor_mtd.name = pl_nor_mtd_name;
	pl_nor_mtd.erasesize = 0; /* would update accordingly later */
	pl_nor_mtd.numeraseregions = num_erase_regions;
	pl_nor_mtd.eraseregions = (struct mtd_erase_region_info *)kmalloc(
				   sizeof(struct mtd_erase_region_info) *
				   num_erase_regions, GFP_KERNEL);
	if ( pl_nor_mtd.eraseregions == NULL ) {
		printk("Failed to allocate MTD erase regions info\n");
		kfree(pl_nor_mtd.name);
		return -1;
	}

do { /* setup MTD erase region */
	int c_size, c_blk, c_idx;
	struct mtd_erase_region_info *p;
	
	c_size = c_blk = 0;
	c_idx = -1; /* make sure we can be right at the first time */
	p = pl_nor_mtd.eraseregions;
	for ( i=0; i<total_sectors; i++ ) {
		if ( nor_sector_map[i].size != c_size ) {
			c_idx ++;
			if ( c_idx >= num_erase_regions ) {
			/* shouldn't be here */
				printk("Shouldn't be here... Wrong!!");
				break;
			}
			p = &(pl_nor_mtd.eraseregions[c_idx]);
			p->offset = nor_sector_map[i].start_addr;
			p->erasesize = c_size = nor_sector_map[i].size;
			p->numblocks = 1;
			
			if ( pl_nor_mtd.erasesize < p->erasesize )
				pl_nor_mtd.erasesize = p->erasesize;
			
			continue;
		}
		p->numblocks ++;
	}
#if 0
	printk("NOR MTD erase regions: \n");
	for ( i=0; i<num_erase_regions; i++ ) {
		struct mtd_erase_region_info *p;
			
		p = &(pl_nor_mtd.eraseregions[i]);
		printk("  Start %u erase size %u total blocks %u\n",
		       p->offset, p->erasesize, p->numblocks);
	}
#endif		
} while (0); 
	
	pl_nor_mtd.module = THIS_MODULE;
	pl_nor_mtd.erase = pl_nor_flash_erase;
	pl_nor_mtd.read = pl_nor_flash_read;
	pl_nor_mtd.write = pl_nor_flash_write;
	
	rc = add_mtd_device(&pl_nor_mtd);
	if ( rc < 0 ) {
		printk("Cannot register NOR MTD, rc = %d\n", rc);
		goto mtd_init_failed;
	}
	
	rc = add_mtd_partitions(&pl_nor_mtd, pl_nor_partitions, 
			 	ARRAY_SIZE(pl_nor_partitions));
	if ( rc < 0 ) {
		printk("Cannot add mtd partitions, rc = %d\n", rc);
		goto mtd_init_failed;
	}
	
	initialized = 1;
	
	return 0;
	
mtd_init_failed:
	cleanup_mtd_struct();
	return -1;	
}

void __exit pl_nor_mtd_exit(void)
{
	if ( initialized == 0 )
		return;
		
	cleanup_mtd_struct();
}

module_init(pl_nor_mtd_init);
module_exit(pl_nor_mtd_exit);

