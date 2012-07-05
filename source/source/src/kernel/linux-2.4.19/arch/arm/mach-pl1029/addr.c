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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include "addr.h"

#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
static devfs_handle_t addr_handle;
static devfs_handle_t plio_handle;
#endif

static int korder = 0;
static unsigned long kaddr = 0;
static unsigned int klen = 0;


#if 0
#define vi_debug(fmt, arg...)  \
    if (debug)  printk(KERN_NOTICE fmt,##arg)
#else
#define vi_debug(fmt, arg...)   /* donothing */
#endif


static int addr_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int addr_release(struct inode *inode, struct file *filp)
{
//    char *p;
//    struct page *page;

    if (kaddr != 0) {
//        for (p = (char *)kaddr + PAGE_SIZE; p < (char *)kaddr + klen; p += PAGE_SIZE) {
//            page = virt_to_page(p);
//            put_page(page);
//        }
        free_pages(kaddr, korder);
    }

    kaddr = 0;

    return 0;
}

static ssize_t addr_read(struct file *filp, char *buf, size_t count, loff_t *offp)
{
    return 0;
}

static ssize_t addr_write(struct file *filp, const char *buf, size_t count, loff_t *offp)
{
    return 0;
}

static int addr_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct port_t port;
    unsigned long paddr = 0;

    if (_IOC_DIR(cmd) & _IOC_READ) {
        if (! access_ok(VERIFY_WRITE, (char *)arg, _IOC_SIZE(cmd)))
            return -EFAULT;
    } else if (_IOC_DIR(cmd) & _IOC_WRITE) {
        if (! access_ok(VERIFY_READ, (char *)arg, _IOC_SIZE(cmd)))
            return -EFAULT;
    }

    if (copy_from_user(&port, (int *)arg, sizeof(struct port_t)))
        return -EFAULT;


    switch(cmd) {
    case ADDR_READL:
        vi_debug("readl %08x\n", port.port);
        if (port.port >= PAGE_OFFSET)
            port.value = readl(port.port);
        else
            port.value = -1;
        if (copy_to_user((void *)arg, &port, sizeof(struct port_t)))
            return -EFAULT;
        break;
    case ADDR_READW:
        vi_debug("readw %08x\n", port.port);
        if (port.port >= PAGE_OFFSET)
            port.value = readw(port.port);
        else
            port.value = -1;

        if (copy_to_user((void *)arg, &port, sizeof(struct port_t)))
            return -EFAULT;
        break;
    case ADDR_READB:
        vi_debug("readb %08x\n", port.port);
        if (port.port >= PAGE_OFFSET)
            port.value = readb(port.port);
        else
            port.value = -1;
        if (copy_to_user((void *)arg, &port, sizeof(struct port_t)))
            return -EFAULT;
        break;
    case ADDR_WRITEL:
        vi_debug("writel %08x = %08x\n", port.port, port.value);
        if (port.port >= PAGE_OFFSET)
            writel(port.value, port.port);
        break;

    case ADDR_WRITEW:
        vi_debug("writew %08x = %04x\n", port.port, (port.value & 0xFFFF));
        if (port.port >= PAGE_OFFSET)
            writew((port.value & 0xFFFF), port.port);
        break;

    case ADDR_WRITEB:
        vi_debug("writeb %08x = %02x\n", port.port, (port.value & 0xFF));
        if (port.port >= PAGE_OFFSET)
            writeb((port.value & 0xFF), port.port);
        break;

    case ADDR_PADDR:
        paddr = virt_to_bus(kaddr);
        ret = put_user(paddr, (int *)arg);
        break;

    default:
        pr_debug("Invalid ioctl %x\n", cmd);
        ret = -ENOTTY;
    }

    return ret;
}

void addr_vma_open(struct vm_area_struct *vma)
{
    MOD_INC_USE_COUNT;
}

void addr_vma_close(struct vm_area_struct *vma)
{
    MOD_DEC_USE_COUNT;
}

static DECLARE_MUTEX(addr_vma_sem);

static struct page *addr_vma_nopage (struct vm_area_struct *vma, unsigned long address, int write_access)
{
    unsigned long offset;
    void *pageptr = NULL;   /* default to "missing" */

    struct page *page = NOPAGE_SIGBUS;
    down(&addr_vma_sem);

    offset = address - vma->vm_start;
    if (offset >= klen)
        goto EXIT;  /* out of range */

    pageptr = (void *)(kaddr + offset);

    page = virt_to_page(pageptr);
    /* got it, now increment the count */
    get_page(page);

#if 0
printk("pageptr 0x%p address 0x%lx vm_start 0x%lx offset %ld kaddr 0x%lx count %d\n",
    pageptr, address, vma->vm_start, offset, jinfo.kaddr, page_count(page));
#endif


EXIT:
    up(&addr_vma_sem);

    return page;
}


static struct vm_operations_struct addr_vm_ops = {
    open:   addr_vma_open,
    close:  addr_vma_close,
    nopage: addr_vma_nopage,
};

static int addr_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int i;
    unsigned long length = vma->vm_end - vma->vm_start;
    char *p;
    struct page *page;

    for (i = 0 ; i < 11; i++) {
        if ((PAGE_SIZE << i) >= length)
            break;
    }
    if (i == 10) {
        printk("The required memory is too big, the size can not be larger than 2M bytes\n");
        return -EINVAL;
    }

    /* map a big memory for i2c driver test */
    kaddr = __get_free_pages(GFP_KERNEL | GFP_DMA, i);
    if (kaddr == 0) {
        printk("Failed to get free pages, order is %d\n", korder);
        return -ENOMEM;
    }
    korder = i;

    klen = (PAGE_SIZE << korder);
    for (p = (char *)kaddr + PAGE_SIZE; p < (char *)kaddr + klen; p += PAGE_SIZE) {
        page = virt_to_page(p);
        get_page(page);
    }

    vma->vm_ops = &addr_vm_ops;
    vma->vm_flags |= VM_RESERVED; /* doesn't swap */
    addr_vma_open(vma);

    return 0;
}



static struct file_operations addr_fops = {
    owner:      THIS_MODULE,
    open:       addr_open,
    release:    addr_release,
    read:       addr_read,
    write:      addr_write,
    ioctl:      addr_ioctl,
    mmap:       addr_mmap,
};


static int plio_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int plio_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t plio_read(struct file *filp, char *buf, size_t count, loff_t *offp)
{
    return 0;
}

static ssize_t plio_write(struct file *filp, const char *buf, size_t count, loff_t *offp)
{
    return 0;
}

static int plio_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long length = vma->vm_end - vma->vm_start;
    vma->vm_pgoff = 0x18000000 >> PAGE_SHIFT;
    vma->vm_flags |= VM_IO | VM_RESERVED;
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    /* remap prolific io space to user virtual memory space */
    if (remap_page_range(vma->vm_start, vma->vm_pgoff << PAGE_SHIFT,
                length, vma->vm_page_prot)) {
        return -EAGAIN;
    }

    return 0;
}



static struct file_operations plio_fops = {
    owner:      THIS_MODULE,
    open:       plio_open,
    release:    plio_release,
    read:       plio_read,
    write:      plio_write,
    mmap:       plio_mmap,
};


#define PLIO_CHAR_MAJOR         202
#define ADDR_CHAR_MAJOR         201
#define DRIVER_DESC             "Prolific addr driver"
#define DRIVER_VERSION          "v0.0.4"

static int __init addr_init(void)
{

#ifdef CONFIG_DEVFS_FS
    if (devfs_register_chrdev(ADDR_CHAR_MAJOR, "addr", &addr_fops)) {
        printk(KERN_NOTICE "Can't allocate major number %d for ADDR Devices\n",
                ADDR_CHAR_MAJOR);
        return -EAGAIN;
    }

    addr_handle = devfs_register(NULL, "addr", DEVFS_FL_DEFAULT,
                        ADDR_CHAR_MAJOR, 0,
                        S_IFCHR | S_IRUGO | S_IWUGO,
                        &addr_fops, NULL);


    if (devfs_register_chrdev(PLIO_CHAR_MAJOR, "plio", &plio_fops)) {
        printk(KERN_NOTICE "Can't allocate major number %d for PLIO Devices\n",
                PLIO_CHAR_MAJOR);
        return -EAGAIN;
    }

    plio_handle = devfs_register(NULL, "plio", DEVFS_FL_DEFAULT,
                        PLIO_CHAR_MAJOR, 0,
                        S_IFCHR | S_IRUGO | S_IWUGO,
                        &plio_fops, NULL);



#else
    if (register_chrdev(ADDR_CHAR_MAJOR, "addr", &addr_fops)) {
        printk(KERN_NOTICE "Can't allocate major number %d for ADDR Devices\n",
                ADDR_CHAR_MAJOR);
        return -EAGAIN;
    }

    if (register_chrdev(PLIO_CHAR_MAJOR, "plio", &plioi_fops)) {
        printk(KERN_NOTICE "Can't allocate major number %d for PLIO Devices\n",
                PLIO_CHAR_MAJOR);
        return -EAGAIN;
    }

#endif
    pr_info(DRIVER_DESC" "DRIVER_VERSION"\n");

    return 0;
}

static void __exit addr_exit(void)
{
#ifdef CONFIG_DEVFS_FS
    devfs_unregister(addr_handle);
    devfs_unregister_chrdev(ADDR_CHAR_MAJOR, "addr");
    devfs_unregister(plio_handle);
    devfs_unregister_chrdev(PLIO_CHAR_MAJOR, "plio");
#else
    unregister_chrdev(ADDR_CHAR_MAJOR, "addr");
    unregister_chrdev(PLIO_CHAR_MAJOR, "plio");
#endif

}


module_init(addr_init);
module_exit(addr_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Jedy Wei");
MODULE_SUPPORTED_DEVICE("Prolifc writel/readl helper");


MODULE_PARM(debug, "i");
MODULE_PARM_DESC(debug, "Debugging option");

