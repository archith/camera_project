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
#define NOR_VERSION	"0.8.3"

#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/dma.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/semaphore.h>

#define MAJOR_NR PL1029_NOR_MAJOR
#define PARTN_BITS 6
/* above two definitions must be defined before including blk.h */
#include <linux/blk.h>
#include <linux/blkpg.h>

#include "pl1029_nor.h"

#define NOR_GET_ERASE_BLOCK	_IOR(0x81, 0x01, unsigned long)

/* nor flash block related */
#define NOR_TYPE_NUM 3
const char *nor_type[NOR_TYPE_NUM] = { "spi", "ppi-amd", "ppi-intel" };
static int norflash_tag = 0;
static struct tag_norinfo nor_geometry;
nor_sector_t nor_sector_map[MAX_NOR_SECTOR];
static int total_sectors = 0;

/* logical partitioning related */
#define MAX_NOR_PHY_PARTS	8
static nor_sector_t nor_block_parts[MAX_NOR_PHY_PARTS];
static nor_sector_t nor_char_parts[MAX_NOR_PHY_PARTS];
static unsigned long nor_char_blocksize_mask[MAX_NOR_PHY_PARTS];
static int total_block_parts = 0;
static int total_char_parts = 0;

/* block device related variables */
static struct hd_struct nor_hd[MAX_NOR_PHY_PARTS<<PARTN_BITS];
static int nor_sizes[MAX_NOR_PHY_PARTS<<PARTN_BITS];
static int nor_blocksizes[MAX_NOR_PHY_PARTS<<PARTN_BITS];
static int nor_hardsectsizes[MAX_NOR_PHY_PARTS<<PARTN_BITS];
static int nor_maxsect[MAX_NOR_PHY_PARTS<<PARTN_BITS];

#ifdef MAX_SECTORS
#undef MAX_SECTORS
#endif
#define MAX_SECTORS 32

#if 0
/* experimental value, not working for now */
#define BLOCK_SIZE_SHIFT	12
#else
#define BLOCK_SIZE_SHIFT	9
#endif
#define NOR_BLOCK_SIZE		(1UL << BLOCK_SIZE_SHIFT)	

/* driver related */
static struct norflash nor;
static u8 intr_status;
static int controller_busy = 0;
static char *dma_buf = NULL;
static int dma_buf_size = 0;

static DECLARE_MUTEX(sem);
static DECLARE_WAIT_QUEUE_HEAD(wq_intr);

/* DMA related variables and functions */

#define MDE_NXT_SHIFT           21
#define MDE_VALID_SHIFT         30
#define MDE_EMDE_SHIFT          31

typedef struct {
        unsigned long saddr; /* starting address */
        unsigned long mde; /* mde flag */
} dma_mde_t; 

#define MDE_MAX_ENTRY	MAX_SECTORS
static dma_mde_t mdt[MDE_MAX_ENTRY] __attribute__ ((aligned (8)));
static struct iovec iovec_tbl[MDE_MAX_ENTRY+1];

static int pl1029_init_dma(void)
{
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

static int pl1029_construct_io_vec(struct request *rq, struct iovec *tbl)
{
        struct buffer_head *bh;
        int count;
 
        bh = rq->bh;
        if ( !bh ) {
                printk("contruct_io_vec has no bh to use?!\n");
                return -1;
        }       
        
        count = 0;
        do {
                if ( count >= MDE_MAX_ENTRY ) {
                        panic("pl1029_nor.c bug! io count >= MDE_MAX_ENTRY");
                }
                tbl[count].iov_base = bh->b_data;
                tbl[count++].iov_len = bh->b_size;
        } while ((bh = bh->b_reqnext) != NULL);
        
        tbl[count].iov_base = NULL;
        tbl[count].iov_len = 0;
        
        return count;
}

static int pl1029_setup_dma(int mode, struct iovec *vecs, int count)
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

static void pl1029_unmap_dma(struct iovec *vecs, int count, int mode)
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

static void pl1029_end_request(struct request *rq, int uptodate)
{
        unsigned long flags;

        while ( end_that_request_first(rq, uptodate, DEVICE_NAME) ) 
                ;
#ifndef DEVICE_NO_RANDOM
        add_blkdev_randomness(MAJOR(rq->rq_dev));
#endif
        spin_lock_irqsave(&io_request_lock, flags);
        end_that_request_last(rq);
        spin_unlock_irqrestore(&io_request_lock, flags);
}

static int pl1029_nor_dma_read(struct request *rq)
{
        int dev, rc, count;
        unsigned long block, nsect;
        unsigned long start_addr, total_len;
        int uptodate = 0;
        
        dev = MINOR(rq->rq_dev);
        block = rq->sector;
        nsect = rq->nr_sectors;

        block += ((nor_block_parts[dev>>6].start_addr)>>BLOCK_SIZE_SHIFT);
        start_addr = (block<<BLOCK_SIZE_SHIFT);
        total_len = (nsect<<BLOCK_SIZE_SHIFT);
        count = pl1029_construct_io_vec(rq, iovec_tbl);
        rc = pl1029_setup_dma(DMA_MODE_READ, iovec_tbl, count);
        if ( rc != total_len ) {
                printk("dma length(%lu) doesn't match, rc=%d\n", total_len, rc);
                return -1;
        }
        
        intr_status = 0x00;
        NOR_INTR_SET_MASK(NOR_CMD_INTR_MASK);
        enable_dma(DMA_PL_NOR);
        nor.ops->cmd_read(start_addr, total_len, DMA_START);
  
        sleep_on(&wq_intr);

        if ( (intr_status & 0x01)==0 && get_dma_residue(DMA_PL_NOR)==0 ) { 
                /* dma done */
                uptodate = 1;
        } else {
                printk("dma error?!\n");
        }
        NOR_INTR_SET_MASK(NOR_INTR_MASKALL);
        disable_dma(DMA_PL_NOR);
        pl1029_unmap_dma(iovec_tbl, count, DMA_MODE_READ);
        pl1029_end_request(rq, uptodate);
        nor.ops->cmd_read(start_addr, total_len, DMA_END);

        return 0;
}                              

static int pl1029_nor_dma_write(struct request *rq)
{
        return 0;
}

/* end of DMA related functions */

static void pl1029_do_nor_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
        intr_status = readb(INTR_S_REG);
#if 0
        printk("NOR INTR: intr_status = 0x%02x\n", intr_status);
#endif        
        wake_up(&wq_intr);

        return;
}

/* Block device related functions */

static int pl1029_norblock_open(struct inode *inode, struct file *filp)
{
        return 0;
}

static int pl1029_norblock_release(struct inode *inode, struct file *filp)
{
        return 0;
}

static int pl1029_norblock_ioctl(struct inode *inode, struct file *filp,
                            unsigned int cmd, unsigned long arg)
{
        return blk_ioctl(inode->i_rdev, cmd, arg);
}

static void pl1029_do_nor_request(request_queue_t *q)
{
        struct request *cur_rq;
        int rc;

        if ( !QUEUE_EMPTY && CURRENT->rq_status == RQ_INACTIVE ) 
                return;

        down(&sem);

        while ( !controller_busy ) {
                controller_busy = 1;
                
                if ( QUEUE_EMPTY ) {
                        controller_busy = 0;
                        up(&sem);
                        return;
                }
                
                cur_rq = CURRENT;
                blkdev_dequeue_request(cur_rq);
                spin_unlock_irq(&io_request_lock);
                
                if ( cur_rq->cmd == READ ) {
                        rc = pl1029_nor_dma_read(cur_rq);
                } else if ( cur_rq->cmd == WRITE ) {
                        rc = pl1029_nor_dma_write(cur_rq);
                } else {
                        printk("NOR: unkwown command = %d\n", cur_rq->cmd);
                        panic("die as you wish\n");                        
                }
                
                spin_lock_irq(&io_request_lock);
                controller_busy = 0;
        }
        up(&sem);
}

static struct block_device_operations nor_bdops = {
        open:		pl1029_norblock_open,
        release:	pl1029_norblock_release,
        ioctl:		pl1029_norblock_ioctl,
};

static struct gendisk nor_gendisk = {
        major:		MAJOR_NR,
        major_name:	"norblock",
        minor_shift:	PARTN_BITS,
        max_p:		1 << PARTN_BITS,
        part:		nor_hd,
        sizes:		nor_sizes,
        fops:		&nor_bdops,
};

/* character device related functions */

static int pl1029_nor_erase(unsigned long addr, int size, int block)
{
        int rc = 0;

        NOR_INTR_SET_MASK(NOR_DMA_INTR_MASK);
        while ( size > 0 ) {
                intr_status = 0x00;
                nor.ops->cmd_erase(addr, DMA_START);
                sleep_on(&wq_intr);
                if ( (intr_status & 0x04) != 0 ) {
                        printk("nor erase sector %lu failed?!\n", addr);
                }
                rc = nor.ops->cmd_erase(addr, DMA_END);
                if ( rc < 0 ) {
                        printk("nor erase data polling error?!\n");
                        break;
                }
                size -= block;
                addr += block;
        }
        NOR_INTR_SET_MASK(NOR_INTR_MASKALL);
        
        return rc;
}

static loff_t pl1029_norchar_llseek(struct file *filp, loff_t offset, 
                                    int whence)
{
        loff_t newpos;
        int minor = MINOR(filp->f_dentry->d_inode->i_rdev);
        int phy_size;
        
        phy_size = nor_char_parts[minor].size;
        
        switch ( whence ) {
        case 0: /* SEEK_SET */
                newpos = offset;
                break;
        case 1: /* SEEK_CUR */
                newpos = filp->f_pos + offset;
                break;
        case 2: /* SEEK END */
                if ( offset == 0 ) {
                        newpos = phy_size;
                        break;
                } else {
                        /* No support to extend size of the device */
                        return -EINVAL;
                }
        default:
                return -EINVAL;
        }
        if ( newpos < 0 )
                return -EINVAL;
        if ( newpos > phy_size )
                return -EINVAL;
        filp->f_pos = newpos;
        
        return newpos;
}

static int pl1029_norchar_open(struct inode *inode, struct file *filp)
{
        return 0;
}

static int pl1029_norchar_release(struct inode *inode, struct file *filp)
{
        return 0;
}

static int pl1029_norchar_read(struct file *filp, char *buf, size_t count,
                               loff_t *f_pos)
{
        int minor = MINOR(filp->f_dentry->d_inode->i_rdev);
        unsigned long start_addr;
        int rc, idx, this_cnt;

        if ( filp->f_pos & nor_char_blocksize_mask[minor] ) {
                printk("norchar%d read position is NOT block-aligned\n", minor);
                return -EFAULT;
        }
        
        if ( count & nor_char_blocksize_mask[minor] ) {
                printk("norchar%d read size is NOT block-aligned\n", minor);
                return -EINVAL;
        }
        
        if ( *f_pos >= nor_char_parts[minor].size ) {
                rc = 0;
                goto out;
        }
        
        if ( (*f_pos+count) > nor_char_parts[minor].size ) {
                count = nor_char_parts[minor].size - *f_pos;
        }

        if ( down_interruptible(&sem) ) {
                return -ERESTARTSYS;
        }

        idx = 0;
        this_cnt = nor_char_blocksize_mask[minor]+1; /* one erase block */
        rc = count;
        while ( count > 0 ) {        
                iovec_tbl[0].iov_base = dma_buf;
                iovec_tbl[0].iov_len = this_cnt;
                if ( pl1029_setup_dma(DMA_MODE_READ, iovec_tbl, 1) != this_cnt ) {
                        printk("norchar%d setup DMA read error\n", minor);
                        rc = -EFAULT;
                        goto out;
                }
        
                intr_status = 0x00;
                NOR_INTR_SET_MASK(NOR_CMD_INTR_MASK);
                enable_dma(DMA_PL_NOR);
                start_addr = nor_char_parts[minor].start_addr + filp->f_pos;
                nor.ops->cmd_read(start_addr, this_cnt, DMA_START);
        
                sleep_on(&wq_intr);
                if ( !((intr_status & 0x01) == 0 && get_dma_residue(DMA_PL_NOR)==0) ) {
                        printk("norchar%d dma read error?!\n", minor);
                        rc = -EIO;
                }
                NOR_INTR_SET_MASK(NOR_INTR_MASKALL);
                disable_dma(DMA_PL_NOR);
                pl1029_unmap_dma(iovec_tbl, 1, DMA_MODE_READ);
                nor.ops->cmd_read(start_addr, this_cnt, DMA_END);
        
                if ( copy_to_user(buf+idx, dma_buf, this_cnt) ) {
                        rc = -EFAULT;
                        goto out;
                }
                *f_pos += this_cnt;
                count -= this_cnt;
                idx += this_cnt;
        }
out:
        up(&sem);
        
        return rc;
}

static int pl1029_norchar_write(struct file *filp, const char *buf, 
                                size_t count, loff_t *f_pos)
{
        int minor = MINOR(filp->f_dentry->d_inode->i_rdev);
        unsigned long start_addr;
        int rc, idx, this_cnt;

        if ( filp->f_pos & nor_char_blocksize_mask[minor]  ) {
                printk("norchar%d write position(%llu) is NOT block-aligned\n", minor, filp->f_pos);
                return -EFAULT;
        }
        
        if ( count & nor_char_blocksize_mask[minor] ) {
                printk("norchar%d write size is NOT block-aligned\n", minor);
                return -EINVAL;
        }

        if ( *f_pos >= nor_char_parts[minor].size ) {
                rc = 0;
                goto out;
        }
        
        if ( (*f_pos+count) > nor_char_parts[minor].size ) {
                count = nor_char_parts[minor].size - *f_pos;
        }

        if ( down_interruptible(&sem) )
                return -ERESTARTSYS;

        idx = 0;
        this_cnt = nor_char_blocksize_mask[minor]+1; /* one erase block */
        rc = count;
        while ( count > 0 ) {
                start_addr = nor_char_parts[minor].start_addr + filp->f_pos;
                if ( pl1029_nor_erase(start_addr, this_cnt, this_cnt) != 0 ) {
                        rc = -EFAULT;
                        goto out;
                }

                if ( copy_from_user(dma_buf, buf+idx, this_cnt) ) {
                        rc = -EFAULT;
                        goto out;
                }

                iovec_tbl[0].iov_base = dma_buf;
                iovec_tbl[0].iov_len = this_cnt;
                if ( pl1029_setup_dma(DMA_MODE_WRITE, iovec_tbl, 1) != this_cnt ) {
                        printk("norchar%d setup DMA write error\n", minor);
                        rc = -EFAULT;
                        goto out;
                }
        
                intr_status = 0x00;
                NOR_INTR_SET_MASK(NOR_CMD_INTR_MASK);
                enable_dma(DMA_PL_NOR);
                nor.ops->cmd_write(start_addr, this_cnt, DMA_START);
        
                sleep_on(&wq_intr);
        
                if ( !((intr_status & 0x01) == 0 && get_dma_residue(DMA_PL_NOR)==0) ) {
                        printk("norchar%d dma write error?!\n", minor);
                        rc = -EIO;
                }
                NOR_INTR_SET_MASK(NOR_INTR_MASKALL);
                disable_dma(DMA_PL_NOR);
                pl1029_unmap_dma(iovec_tbl, 1, DMA_MODE_WRITE);
                nor.ops->cmd_write(start_addr, this_cnt, DMA_END);
                *f_pos += this_cnt;
                idx += this_cnt;
                count -= this_cnt;
        }
out:
        up(&sem);
        
        return rc;
}                                

static int pl1029_norchar_ioctl(struct inode *inode, struct file *filp,
                                unsigned int cmd, unsigned long arg)
{
        int rc = 0;
        int minor = MINOR(inode->i_rdev);
        
        if ( minor > total_char_parts )
                return -EINVAL;
        
        switch (cmd) {
        case NOR_GET_ERASE_BLOCK:
                rc = nor_char_blocksize_mask[minor] + 1;
                break;
        default:
                return -EINVAL;
        }

        return copy_to_user((void *)arg, &rc, sizeof(unsigned long)) ? -EFAULT : 0;
}                                

static struct file_operations norchar_fops = {
        llseek:		pl1029_norchar_llseek,
        read:		pl1029_norchar_read,
        write:		pl1029_norchar_write,
        ioctl:		pl1029_norchar_ioctl,
        open:		pl1029_norchar_open,
        release:	pl1029_norchar_release
};

static void __init pl1029_nor_banner(void)
{
        printk("PL-1029 NOR flash driver, version %s\n", NOR_VERSION);
}

static int __init pl1029_init_nor_controller(void)
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

static int __init pl1029_generate_nor_map(void)
{
        extern pl1029_nor_ops ppi_amd;
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
                if ( (size<<10) > dma_buf_size )
                        dma_buf_size = (size<<10);
                for ( j=0; j<nor_geometry.sectinfo[i].quantum; j++ ) {
                        nor_sector_map[sec_idx].start_addr = addr;
                        nor_sector_map[sec_idx].size = (size<<10);
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
        memset(&nor, 0x00, sizeof(struct norflash));
        nor.type = nor_geometry.type;
        switch ( nor.type ) {
                case 0: /* spi */
                        printk("NOR: spi not supported now\n");
                        return -1;
                case 1: /* ppi amd */
                        nor.ops = &ppi_amd;
                        nor.ops->init();
                        break;
                case 2: /* ppi intel */
                        printk("NOR: ppi intel not supported now\n");
                        return -1;
                default:
                        printk("NOR: unknown flash type\n");
                        return -1;
        }
        nor.ops->read_id(&id);
        printk("NOR flash id = 0x%02x\n", id);

        return 0;
}

static int __init pl1029_init_nor_irq(void)
{
        int rc;
        
        intr_status = readb(INTR_S_REG); /* in case there are still pending intr */
        
        rc = request_irq(IRQ_PL_NOR, pl1029_do_nor_interrupt, 0, "nor", NULL);
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

static void pl1029_register_block_device(void)
{
        int drive;

        if ( devfs_register_blkdev(MAJOR_NR, "nor", &nor_bdops) ) {
                printk("nor: cannot register major %d for driver\n", MAJOR_NR);
                return;
        }                
        blk_init_queue(BLK_DEFAULT_QUEUE(MAJOR_NR), DEVICE_REQUEST);
        add_gendisk(&nor_gendisk);
        
        for ( drive=0; drive<(MAX_NOR_PHY_PARTS<<PARTN_BITS); drive++ ) {
                nor_blocksizes[drive] = NOR_BLOCK_SIZE;
                nor_hardsectsizes[drive] = NOR_BLOCK_SIZE;
                nor_maxsect[drive] = MAX_SECTORS;
        }
        blksize_size[MAJOR_NR] = nor_blocksizes;
        hardsect_size[MAJOR_NR] = nor_hardsectsizes;
        max_sectors[MAJOR_NR] = nor_maxsect;
      
        for ( drive=0; drive<total_block_parts; drive ++ ) {
                nor_hd[drive<<PARTN_BITS].nr_sects = 
                       (nor_block_parts[drive].size)>>BLOCK_SIZE_SHIFT;
        }
        nor_gendisk.nr_real = total_block_parts;

        for ( drive=0; drive<nor_gendisk.nr_real; drive++ ) {
                register_disk(&nor_gendisk, MKDEV(MAJOR_NR, drive<<PARTN_BITS),
                              1<<PARTN_BITS, &nor_bdops, 
                              nor_hd[drive<<PARTN_BITS].nr_sects);
        }
}

static void pl1029_register_char_device(void)
{
        int i;
        char dev_name[16];

        if ( devfs_register_chrdev(MAJOR_NR, "norchar", &norchar_fops) ) {
                printk("Cannot get major %d for NOR character device\n", MAJOR_NR);
                return;
        }
        
        for ( i=0; i<total_char_parts; i++ ) {
                int mask, size=0, j;
                sprintf(dev_name, "norchar%d", i);
                devfs_register(NULL, dev_name, DEVFS_FL_DEFAULT, MAJOR_NR,
                               i, S_IFCHR|S_IRUGO|S_IWUGO, &norchar_fops, 
                               NULL);
                mask = 1;
                for ( j=0; j<total_sectors; j++ ) {
                        if ( ( nor_char_parts[i].start_addr >= 
                               nor_sector_map[j].start_addr    ) &&
                             ( nor_char_parts[i].start_addr < 
                               nor_sector_map[j].start_addr + 
                               nor_sector_map[j].size          ) ) {
                                size = nor_sector_map[j].size;
                                break;
                        }
                }
                if ( size == 0 ) {
                        printk("norchar%d is not a valid start_addr\n", i);
                        continue;
                }
                while ( (size & mask) == 0 ) {
                        mask <<= 1;
                }
                nor_char_blocksize_mask[i] = (mask-1);
        }
}

int __init pl1029_nor_init(void)
{
        pl1029_nor_banner();
        if ( !norflash_tag ) {
                printk("No nor flash tag passed, failed to initialize\n");
                return -1;
        }
        pl1029_init_nor_controller();
        pl1029_generate_nor_map();
        
        if ( pl1029_init_dma()<0 )
                return -1;

        if ( pl1029_init_nor_irq()<0 )
                return -1;

	/* FIX: if we skip the kernel options - "nor=", then there's no
	 *      chance to call such function. I put it here - before register
	 *      block devices */
	pl1029_parse_nor_partition(); /* Vincent add */

        if ( total_block_parts > 0 )
                pl1029_register_block_device();
        if ( total_char_parts > 0 )
                pl1029_register_char_device();

#ifdef CONFIG_PL1029_NOR_UPGRADE
        do {
                extern void pl1029_register_char_upgrader(int, int);
                
                pl1029_register_char_upgrader(MAJOR_NR, MAX_NOR_PHY_PARTS);
        } while (0);
#endif

	return 0;
}

/* kernel tag and parameter helper functions */

static int __init pl1029_parse_tag_norinfo(const struct tag *tag)
{
        memcpy(&nor_geometry, &(tag->u), sizeof(nor_geometry));
        norflash_tag = 1;
        
        return 0;
}
__tagtable(ATAG_NORINFO, pl1029_parse_tag_norinfo);

/* 
 * Kernel parameter to define on-board NOR flash logical partitions.
 *
 * nor=type:size@start_address,...
 *
 * type is one character, either 'b' for block device or 'c' for character
 * device.
 *
 * nor=b:0x100000@0x1d0000,c:16386@16384,c:0x6000@32768
 *
 * would create a block device starts at 0x1d0000 with size 0x100000, and
 * a character device starts at 16384 with size 16384, and the other is
 * at 32768 with size 0x6000
 *
 */
#if 0				/* hard-coded in "memconfig.h" */
static int __init pl1029_parse_nor_partition(char *line)
{
        char *c, *p, type;
        unsigned long start_addr;
        unsigned int size;
        nor_sector_t *nor_parts;
        int idx, failsafe_cnt = 0;

        c = line;
        while ( *c != '\0' ) {
                if ( ++failsafe_cnt > MAX_NOR_PHY_PARTS ) {
                        printk("WARNING: max. NOR partitions overflow, ignore the rest\n");
                        break;
                }
                type = *c++;
                if ( *c != ':' ) {
                        printk("syntax of NOR kernel parameter error!");
                        break;
                }
                p = ++c;
                while ( *p != '@' && *p != '\0' ) p++;
                if ( *p == '\0' ) {
                        printk("ERR: need start address of NOR partition\n");
                        break;
                }
                *p++ = '\0';
                size = simple_strtoul(c, NULL, 0);
                c = p;
                while ( *p != ',' && *p != '\0' ) p++;
                if ( *p != '\0' )
                        *p++ = '\0';
                start_addr = simple_strtoul(c, NULL, 0);
                c = p;

                if ( type == 'b' ) {
                        nor_parts = nor_block_parts;
                        idx = total_block_parts++;
                } else if ( type == 'c' ) {
                        nor_parts = nor_char_parts;
                        idx = total_char_parts++;
                } else {
                        printk("unknown NOR partition type 0x%02x\n", type);
                        continue;
                }
                nor_parts[idx].start_addr = start_addr;
                nor_parts[idx].size = size;
                failsafe_cnt++;
        }

        return 1;
}
__setup("nor=", pl1029_parse_nor_partition);
#else
static int __init pl1029_parse_nor_partition(void)
{
        nor_sector_t *nor_parts;
        int idx;

//	if (BSPCONF_FLASH_PARTITIONS > MAX_NOR_PHY_PARTS) {
//		printk("WARNING: max. NOR partitions [%d] overflow [>%d], ignore the rest\n", BSPCONF_FLASH_PARTITIONS, MAX_NOR_PHY_PARTS);
//	}
#if (BSPCONF_FLASH_PARTITIONS > MAX_NOR_PHY_PARTS)
#error MAX NOR partitions overflow!!
#endif

	/* 2. BLOCK device for different partitions: FS... */
	/* -- File System -- */
	nor_parts = nor_block_parts;
	idx = total_block_parts++;
	nor_parts[idx].start_addr = BSPCONF_FS_FLASH_OFFSET;
	nor_parts[idx].size = BSPCONF_FS_FLASH_SIZE;
	/* 2. CHAR device for different partitions: MAC/CONF/HTTPS... */
	/* -- MAC -- */
	nor_parts = nor_char_parts;
	idx = total_char_parts++;
	nor_parts[idx].start_addr = BSPCONF_MAC_FLASH_OFFSET;
	nor_parts[idx].size = BSPCONF_MAC_FLASH_SIZE;
	/* -- CONF -- */
	nor_parts = nor_char_parts;
	idx = total_char_parts++;
	nor_parts[idx].start_addr = BSPCONF_CONF_FLASH_OFFSET;
	nor_parts[idx].size = BSPCONF_CONF_FLASH_SIZE;
	/* -- HTTPS -- */
	nor_parts = nor_char_parts;
	idx = total_char_parts++;
	nor_parts[idx].start_addr = BSPCONF_HTTPS_FLASH_OFFSET;
	nor_parts[idx].size = BSPCONF_HTTPS_FLASH_SIZE;
	/* -- KERNEL -- */
	nor_parts = nor_char_parts;
	idx = total_char_parts++;
	nor_parts[idx].start_addr = BSPCONF_KERNEL_FLASH_OFFSET;
	nor_parts[idx].size = BSPCONF_KERNEL_FLASH_SIZE;

        return 1;
}
#endif /* hard-coded in "memconfig.h" */

/* 
 * helper functions to be used in firmware upgrade.
 */

int pl1029_map_sector(unsigned long addr, nor_sector_t *sector)
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

int pl1029_nor_direct_write(char *data, unsigned long start_addr, 
                            int size)
{
        int rc, idx, this_cnt;
        unsigned long addr;

        if ( down_interruptible(&sem) )
                return -ERESTARTSYS;

        idx = 0;
        rc = size;
        addr = start_addr;
        while ( size > 0 ) {
                nor_sector_t sector;

                if ( pl1029_map_sector(addr, &sector) < 0 ) {
                        return rc;
                }

                if ( addr != sector.start_addr ) {
                        printk("firmware upgrade: 0x%lx is not a sector start(0x%lx)\n", addr, sector.start_addr);
                        return -EINVAL;
                }
                if ( sector.size > size ) { 
                /* 
                 * the last remaining data, probably will not multiple of
                 * 16 bytes, which is our DMA line burst length. So, we
                 * need to take care of the padding here.
                 */
                        this_cnt = (size&~0xf) + 16;
#if 0
printk("padding from %d(0x%x) to %d(0x%x)\n", size, size, this_cnt, this_cnt);
#endif
                } else {
                        this_cnt = sector.size;
                }
#if 0
printk("ERASE: 0x%lx, size = %lu, block_size = %d\n", addr, this_cnt, sector.size);
#endif
                if ( pl1029_nor_erase(addr, this_cnt, sector.size) != 0 ) {
                        rc = -EFAULT;
                        goto out;
                }

                if ( copy_from_user(dma_buf, data+idx, this_cnt) ) {
                        rc = -EFAULT;
                        goto out;
                }

                iovec_tbl[0].iov_base = dma_buf;
                iovec_tbl[0].iov_len = this_cnt;
                if ( pl1029_setup_dma(DMA_MODE_WRITE, iovec_tbl, 1) != this_cnt ) {
                        printk("Firmware writing error while setup DMA\n");
                        rc = -EFAULT;
                        goto out;
                }
        
                intr_status = 0x00;
                NOR_INTR_SET_MASK(NOR_CMD_INTR_MASK);
                enable_dma(DMA_PL_NOR);
                nor.ops->cmd_write(addr, this_cnt, DMA_START);
        
                sleep_on(&wq_intr);
        
                if ( !((intr_status & 0x01) == 0 && get_dma_residue(DMA_PL_NOR)==0) ) {
                        printk("Firmware writing error in DMA, "
                               "status = 0x%02x, dma residue = %d\n", intr_status, 
                               get_dma_residue(DMA_PL_NOR));
                        rc = -EIO;
                }
                NOR_INTR_SET_MASK(NOR_INTR_MASKALL);
                disable_dma(DMA_PL_NOR);
                pl1029_unmap_dma(iovec_tbl, 1, DMA_MODE_WRITE);
                nor.ops->cmd_write(addr, this_cnt, DMA_END);
        /* 
         * Though because of the padding in the last remaining data,
         * "this_cnt" is altered, and the following caculation would 
         * be inaccurate, but, "size" would be less than 0 and surely
         * would break the loop. 
         */
                idx += this_cnt;
                size -= this_cnt;
                addr += this_cnt;
        }
out:
        up(&sem);

        return rc;        
}

