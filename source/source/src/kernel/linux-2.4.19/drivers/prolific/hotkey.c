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
static char *hotkey_name = "QPV1 HotKey";
static char *hotkey_version = "1.0.0";
static char *hotkey_revdate = "2004-07-02";


#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

#include <asm/io.h>


/*
 * definitions
 */
#define HOTKEY_USBD_MASS	0
#define HOTKEY_USBD_DOWNLOAD	1


/*
 * variables
 */

static int hotkey_usbd = HOTKEY_USBD_MASS;	/* 0:mass, 1:download */


/*
 * hotkey /proc Interface
 */

/*
 * forward references
 */
static int hotkey_read_proc(char *page, char **start, off_t off,
			    int count, int *eof, void *data);


/*
 * functions
 */

/*
 * only for Quanta USBD mass storage/USBD download selection (cwc)
 * 
 *	GPIO start from 0,1,2,..,15
 *	Caution !! Do not press the following GPIO combinations
 *	at the same time !
 *	GPIO [1,3,4,6], GPIO [2,4,6], GPIO[1,2,4,6]
 */
void
hotkey_init(void)
{
#define GPIO_BASE       0xb94c0000
#define GPIO_ENB        (GPIO_BASE + 0x54)
#define GPIO_DO         (GPIO_BASE + 0x56)  /* Data Output */
#define GPIO_OE         (GPIO_BASE + 0x58)  /* 1:Output/0:Input Selection */
#define GPIO_DI         (GPIO_BASE + 0x5c)  /* Data Input (ReadOnly) */
#define GPIO_KEY_ENB    (GPIO_BASE + 0x78)  /* Keypad enable */
#define GPIO_KEY_POL    (GPIO_BASE + 0x7a)  /* Keypad input select
					     * 0: rasing edge 1: falling edge */

#define GPIO_KEY_MASK   (GPIO_BASE + 0x80)	/* Interupt Mask */
#define GPIO_PF0_PU     (GPIO_BASE + 0x64)
#define GPIO_PF0_PD     (GPIO_BASE + 0x66)
#define GPIO_PF1_SMT    (GPIO_BASE + 0x68)

#define GPIO_PressA	(1L << 7)	/* GPIO7  pressed */
#define GPIO_PressB	(1L << 8)	/* GPIO8  pressed */
#define GPIO_PressC	(1L << 10)	/* GPIO10 pressed */

	unsigned int Inputs;
	unsigned int PressButton;

	writew(0, GPIO_KEY_MASK);	/* Disable GPIO Interrupt */
	writew(0, GPIO_KEY_ENB);	/* Disable Keypad, input active
					 * high(outside view) */
	writew(0, GPIO_DO);		/* No output */
	writew(0, GPIO_OE);		/* Disable output enable, 
					 * setup all I/O to input */
	writel(0, GPIO_PF0_PU);		/* No need for enable pull UP/Down */
	writel(0xFFFF, GPIO_PF1_SMT);	/* enable schmitt trigger for
					 * input pins */

	Inputs = readw(GPIO_DI);
	PressButton = (GPIO_PressB | GPIO_PressC);

	if ((Inputs & PressButton) == PressButton)
		hotkey_usbd = HOTKEY_USBD_DOWNLOAD;
	else
		hotkey_usbd = HOTKEY_USBD_MASS;
} /* hotkey_init() */

int
hotkey_get(void)
{
	return hotkey_usbd;
} /* hotkey_which() */

static int __init
__hotkey_init(void)
{
	create_proc_read_entry("driver/hotkey_usbd", 0, 0, hotkey_read_proc,
				NULL);

	printk(KERN_INFO "%s version %s (%s) --> [%d]\n",
			hotkey_name, hotkey_version, hotkey_revdate,
			hotkey_usbd);

	return 0;
} /* _hotkey_init() */

static int
hotkey_proc_output(char *buf)
{
	char *p;

	p = buf;

	p += sprintf(p, "%d\n", hotkey_usbd);

	return p - buf;
} /* hotkey_proc_output() */

static int
hotkey_read_proc(char *page, char **start, off_t off,
	      int count, int *eof, void *data)
{
	int len = hotkey_proc_output(page);

	if (len <= off+count) *eof = 1;

	*start = page + off;
	len -= off;
        if (len > count) len = count;
	if (len < 0) len = 0;

	return len;
} /* hotkey_read_proc() */

module_init(__hotkey_init);

EXPORT_SYMBOL(hotkey_init);
EXPORT_SYMBOL(hotkey_get);
