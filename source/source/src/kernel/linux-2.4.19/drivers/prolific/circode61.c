/*
*
* device name : /dev/circode
*
* Copyright 2003 Prolific Technology INC.
*
*
* lircd daemon will wrtie scancodes to /dev/circode, then eframe2 could get scancodes
* from this device.
*
*
*/
/*-----------------------------------------------------------------------------------*/
/*                                                                                   */
/* 2004/04/24     Samson Su     create this file                                     */
/*                                                                                   */
/*-----------------------------------------------------------------------------------*/
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
#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/sysctl.h>
#include <linux/console.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/tqueue.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/tty.h>
#include <linux/kd.h>
#include <linux/kbd_ll.h>
#include <linux/kbd_kern.h>
#include <linux/apm_bios.h>
#include <linux/kmod.h>


#define CIR_SCANCODE_MODULE_NAME  "cir_scancode"
//#define CIR_SCANCODE_MAJOR 1  //the major number of misc
#define CIR_SCANCODE_MINOR 64


MODULE_AUTHOR("Samson Su");
MODULE_DESCRIPTION("Prolific PL0161 CIR SCANCODE driver");


static ssize_t circode61_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	unsigned char scancode;
	if ( copy_from_user((void *)&scancode, (void *)buf, sizeof(unsigned char))) {
		return -EFAULT;
	}

	//printk(KERN_INFO "circode61:scancode=[0x%x]\n", scancode);

	handle_scancode(scancode, 1);
	tasklet_schedule(&keyboard_tasklet);


	return sizeof(int);
}


static int circode61_release(struct inode * inode, struct file * filp)
{
	MOD_DEC_USE_COUNT;
        return 0;
}


static int circode61_open( struct inode * inode, struct file * filp)
{
	MOD_INC_USE_COUNT;
	return 0;
}


struct file_operations circode_fops = {
	write:          circode61_write,
	open:           circode61_open,
	release:        circode61_release,
};


/***********************************************************************************/
/*       Initialization                                                            */
/***********************************************************************************/

static devfs_handle_t devfs_circode_dev;
static int scancodeMajor;

int __init circode61_init(void)
{
        printk(KERN_INFO "circode61: init CIR SCANCODE module\n");

        scancodeMajor = devfs_register_chrdev(0, CIR_SCANCODE_MODULE_NAME, &circode_fops);
        if (scancodeMajor < 0) {
                printk(KERN_WARNING ": CIR SCANCODE module can't get major number\n");
                return scancodeMajor;
        }
	//cir scanscode use device "/dev/circode"
	devfs_circode_dev = devfs_register( NULL, "circode", DEVFS_FL_DEFAULT,
				       scancodeMajor, CIR_SCANCODE_MINOR,
				       S_IFCHR | S_IRUSR | S_IWUSR,
				       &circode_fops, NULL);

	return 0;
}

void circode61_exit(void)
{
	printk(KERN_INFO ":  release CIR SCANCODE module\n");
	devfs_unregister(devfs_circode_dev);
	devfs_unregister_chrdev(scancodeMajor, CIR_SCANCODE_MODULE_NAME);
}

module_init(circode61_init);
module_exit(circode61_exit);

