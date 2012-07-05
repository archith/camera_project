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
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/ioctl.h>
#include <linux/reboot.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/dma.h>
#include <asm/irq.h>
#include <asm/hardware.h>

#include "pl1029_nor.h"

#define NOR_SET_UPGRADE	_IOW(0x81, 0x02, unsigned long)

static int can_go = 0;
static int my_minor;

extern int pl1029_nor_direct_write(char *, unsigned long, int);

static struct timer_list reboot_timer;

static void firmware_reboot(unsigned long dummy)
{
        /* reboot */
        printk("firmware upgrade done. Rebooting the system...\n");
        machine_restart(NULL);
}

/*
 * The firmware format.
 *
 * The first block is a program segment of variable length. The size of
 * the program stores at 0x04 for 4 bytes long. And, the other variables
 * also store at,
 *
 * 0x08(size 4 bytes)  -- version number.
 * 0x0c(size 16 bytes) -- MD5 checksum.
 * 0x1c(size 4 bytes)  -- CRC checksum.
 *
 * Though, in the driver level, these three variables are NOT used.
 *
 * The second block is fixed to 4 k-bytes. It contains firmware layout
 * descriptors.
 *
 * Descriptors are two unsigned long numbers. The first one indicates
 * the starting address in the NOR flash layout, and the second one
 * is the size of this part of firmware.
 *
 * The following block is the data block, which the actual raw data
 * packed together.
 *
 */
#define PG_SIZE_OFFSET	0x04
#define FW_HEADER_SIZE	4096
static int pl1029_no_return_upgrade(char *buf, size_t count)
{
        unsigned long start_addr;
        int pg_size, size;
        int done = 0, rc;
        char *num_ptr, *data_ptr;
        
        pg_size = *((int *)(buf+PG_SIZE_OFFSET));
        if ( pg_size == 0 ) {
                return -EINVAL;
        }
        
        num_ptr = buf + pg_size;
        data_ptr = num_ptr + FW_HEADER_SIZE;
        while ( !done ) {
                start_addr = *((unsigned long *)num_ptr);
                size = *((unsigned long *)(num_ptr+4));
                num_ptr += 8;
                if ( start_addr == 0 && size == 0 ) {
                        done = 1; /* successfully done */
                        break; 
                }
                if ( (data_ptr + size) - buf > count ) {
                        printk("firmware descriptor error, overflow...\n");
                        break;
                }
#if 0
printk("NOR Firmware write: 0x%lx, size = %lu\n", start_addr, size);
#endif
                rc = pl1029_nor_direct_write(data_ptr, start_addr, size);
                if ( rc < 0 )
                        break;
                data_ptr += size;
        }
        
        if ( !done ) {
                return -EFAULT;
        }

        /* initiate the reboot timer in one second */
        init_timer(&reboot_timer);
        reboot_timer.function = firmware_reboot;
        reboot_timer.expires = jiffies + 1*HZ;
        add_timer(&reboot_timer);
                
        return 0;
}

static int pl1029_nor_upgrade_open(struct inode *inode, struct file *filp)
{
        can_go = 0;

        return 0;
}

static int pl1029_nor_upgrade_release(struct inode *inode, struct file *filp)
{
        return 0;
}

static int pl1029_nor_upgrade_read(struct file *filp, char *buf,
                                   size_t count, loff_t *f_pos)
{
        return -EFAULT;
}                                   

static int pl1029_nor_upgrade_write(struct file *filp, const char *buf, 
                                    size_t count, loff_t *f_pos)
{
        int rc;
        
        if ( !can_go ) {
                printk("You don't say the magic word!\n");
                return -EINVAL;
        }
        
        rc = pl1029_no_return_upgrade((char *)buf, count);
        if ( rc < 0 )
                return -EFAULT;
        
        return count;
}

static int pl1029_nor_upgrade_ioctl(struct inode *inode, struct file *filp,
                                    unsigned int cmd, unsigned long arg)
{
        int minor = MINOR(inode->i_rdev);
        unsigned long magic = 0;
        
        if ( minor != my_minor ) {
                return -EINVAL;
        }
        
        switch (cmd) {
        case NOR_SET_UPGRADE:
                copy_from_user(&magic, (void *)arg, sizeof(unsigned long));
                if ( magic == 0xdeadbeef ) {
                        can_go = 1;
                } else {
                        can_go = 0;
                        return -EINVAL;
                }
                break;
        default:
                return -EINVAL;
        }
        
        return 0;
}                                    

static struct file_operations fw_fops = {
        read:		pl1029_nor_upgrade_read,
        write:		pl1029_nor_upgrade_write,
        ioctl:		pl1029_nor_upgrade_ioctl,
        open:		pl1029_nor_upgrade_open,
        release:	pl1029_nor_upgrade_release
};

void __init pl1029_register_char_upgrader(int major, int minor)
{
        my_minor = minor;

        devfs_register(NULL, "fwupdate", DEVFS_FL_DEFAULT, major, minor,
                       S_IFCHR|S_IRUGO|S_IWUGO, &fw_fops, NULL);
}

