/*
 *	Watchdog driver for the FTWDT101 Watch Dog Driver
 *
 *      (c) Copyright 2004 Faraday Technology Corp. (www.faraday-tech.com)
 *	    Based on sa1100_wdt.c by Oleg Drokin <green@crimea.edu>
 *          Based on SoftDog driver by Alan Cox <alan@redhat.com>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *      27/11/2004 Initial release
 */

#include <linux/module.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/reboot.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/bitops.h>
#include <asm/io.h>
#define TIMER_MARGIN	10		/* (secs) Default is 1 minute */
static int ftwdt101_margin = TIMER_MARGIN;	/* in seconds */
static int ftwdt101wdt_users;
static int pre_margin;
#define WdLoad		0x4
#define WdRestart	0x8
#define WdCR		0x0C

#ifdef MODULE
MODULE_PARM(ftwdt101_margin,"i");
#endif

/*
 *	Allow only one person to hold it open
 */

static int ftwdt101dog_open(struct inode *inode, struct file *file)
{
	if(test_and_set_bit(1,&ftwdt101wdt_users))
		return -EBUSY;
	MOD_INC_USE_COUNT;
	/* Activate FTWDT101 Watchdog timer */
	//printk("Activating WDT..\n");
	pre_margin=32768 * ftwdt101_margin;
	outb(0, CPE_WDT_VA_BASE+WdCR);
	outl(pre_margin, CPE_WDT_VA_BASE+WdLoad);
	outl(0x5ab9, CPE_WDT_VA_BASE+WdRestart); /*Magic number*/
	outb(0x12,CPE_WDT_VA_BASE+WdCR); /*Use EXTCLK(32768Hz), reset when timeout*/
	outb(0x13, CPE_WDT_VA_BASE+WdCR); /*Enable WDT */
	return 0;
}

static int ftwdt101dog_release(struct inode *inode, struct file *file)
{
	/*
	 *	Shut off the timer.
	 * 	Lock it in if it's a module and we defined ...NOWAYOUT
	 */
	outl(0x5ab9, CPE_WDT_VA_BASE+WdRestart);
#ifndef CONFIG_WATCHDOG_NOWAYOUT
	outb(0, CPE_WDT_VA_BASE+WdCR);
#endif
	ftwdt101wdt_users = 0;
	MOD_DEC_USE_COUNT;
	return 0;
}

static ssize_t ftwdt101dog_write(struct file *file, const char *data, size_t len, loff_t *ppos)
{
	/*  Can't seek (pwrite) on this device  */
	if (ppos != &file->f_pos)
		return -ESPIPE;

	/* Refresh OSMR3 timer. */
	if(len) {
		outl(0x5ab9, CPE_WDT_VA_BASE+WdRestart);
		return 1;
	}
	return 0;
}

static int ftwdt101dog_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	static struct watchdog_info ident = {
		identity: "FTWDT101 Watchdog",
	};

	switch(cmd){
	default:
		return -ENOIOCTLCMD;
	case WDIOC_GETSUPPORT:
		return copy_to_user((struct watchdog_info *)arg, &ident, sizeof(ident));
	case WDIOC_GETSTATUS:
		return put_user(0,(int *)arg);
#if 0
	case WDIOC_GETBOOTSTATUS:
		return put_user((RCSR & RCSR_WDR) ? WDIOF_CARDRESET : 0, (int *)arg);
#endif
	case WDIOC_KEEPALIVE:
		outl(0x5ab9, CPE_WDT_VA_BASE+WdRestart);
		return 0;
	}
}

static struct file_operations ftwdt101dog_fops=
{
	owner:		THIS_MODULE,
	write:		ftwdt101dog_write,
	ioctl:		ftwdt101dog_ioctl,
	open:		ftwdt101dog_open,
	release:	ftwdt101dog_release,
};

static struct miscdevice ftwdt101dog_miscdev=
{
	WATCHDOG_MINOR,
	"FTWDT101 watchdog",
	&ftwdt101dog_fops
};

static int __init ftwdt101dog_init(void)
{
	int ret;

	ret = misc_register(&ftwdt101dog_miscdev);

	if (ret)
		return ret;

	printk("FTWDT101 Watchdog Timer: timer margin %d sec\n", ftwdt101_margin);

	return 0;
}

static void __exit ftwdt101dog_exit(void)
{
	misc_deregister(&ftwdt101dog_miscdev);
}

module_init(ftwdt101dog_init);
module_exit(ftwdt101dog_exit);
