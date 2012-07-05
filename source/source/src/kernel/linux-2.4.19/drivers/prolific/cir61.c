/*
* PL1061 Consumer Infrared Driver
*
* device name : /dev/lirc/0
*
* Copyright 2003 Prolific Technology INC.
*
* PL1061B support three CIR protocol, they are NEC, Tatung, and Software detect(protocol
* is detected by software),We use software detect mode because we want to integrate this
* dirver with LIRC Package, this driver uses GPIO[8] as a CIR TX output port and uses
* GPIO[9] as a CIR RX input port(now, Pl1061B just support RX input)
*
*/

/*-----------------------------------------------------------------------------------*/
/*                                                                                   */
/* 2003/07/03     Samson Su     1. Initial Release(for PL1061B)                      */
/*                              2. Keypad, touch screen and CIR share                */
/*                                 some registers                                    */
/*                              3. Keypad and CIR share the same IRQ number          */
/*                                                                                   */
/* 2004/04/14     Samson Su     1. add this driver to eframe package, change file    */
/*                                 name from pl1061_cir.c to cir61.c                 */
/*                              2. unsigned long long for timestamp calculation      */
/*                                                                                   */
/* 2004/04/24     Jedy          tune this file                                       */
/*                                                                                   */
/* 2005/05/26     Samson Su     support pl1063                                       */
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
//#include <asm/pl1061/pl1061.h>
//#include <asm/pl1061/irq.h>
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
#include <linux/kbd_ll.h>
#include <linux/apm_bios.h>
#include <linux/kmod.h>

#define PULSE_BIT  0x01000000
#define PULSE_MASK 0x00FFFFFF

typedef int lirc_t;

#undef PDEBUG		/* undef it, just in case */
//#define PDEBUG(fmt, args...) printk("LIRC1061: %s: " fmt, __FUNCTION__ , ## args)
//#define PDEBUG2(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)
#define PDEBUG(fmt, args...)
#define PDEBUG2(fmt, args...)

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */

#undef PDEBUGG2
#define PDEBUGG2(fmt, args...)



// keypad registers
#define reGPIO_BASE			0xB94C0000
#define reTCFG0				( 0xB94C0000 + 0x00 )
#define	reTCFG1				( 0xB94C0000 + 0x04 )
#define	reTCON				( 0xB94C0000 + 0x10 )
#define reGPIO_OE			( 0xB94C0000 + 0x54 )
#define reLED_CONTROL			( 0xB94C0000 + 0x54 )
#define reGPIO_OD			( 0xB94C0000 + 0x58 )
#define reGPIO_IR			( 0xB94C0000 + 0x5C )
#define reGPIO_OW			( 0xB94C0000 + 0x60 )
#define reGPIO_PF0			( 0xB94C0000 + 0x64 )
#define reGPIO_PF1			( 0xB94C0000 + 0x68 )
#define reGPIO_KEYREG			( 0xB94C0000 + 0x78 )
#define reGPIO_KEY0REG			( 0xB94C0000 + 0x7c )
#define reGPIO_KEY1REG			( 0xB94C0000 + 0x80 )

// CIR registers
#define	rCIR0				( 0xB94C0000 + 0x84 )
#define	rCIR1				( 0xB94C0000 + 0x88 )
#define	rCIR2				( 0xB94C0000 + 0x8c )
#define	rCIR3				( 0xB94C0000 + 0x90 )
#define	rCIR4				( 0xB94C0000 + 0x94 )
#define	rCIR_NEC			( 0xB94C0000 + 0x98 )
#define	rCIR_TATUNG			( 0xB94C0000 + 0x9c )
#define	rCIR_SW0			( 0xB94C0000 + 0xa0 )
#define	rCIR_SW1			( 0xB94C0000 + 0xa4 )


#define RSTN_CIR        (1L << (29-16))
#define RSTN_KEY        (1L << (30-16))
#define RSTN_PWM        (1L << (31-16))

#ifdef CONFIG_PL1063
#define RSTN_INTX       (1L << (28-16))
#endif

//Time Stamp registers
#define rFTSTP_SECOND			0xBB00022C
#define rFTSTP_FRACTIONAL		0xBB000228


//copy from lirc package(...drivers/lirc.h)
/*
 * lirc compatible hardware features
 */
#define LIRC_MODE2SEND(x) (x)
#define LIRC_SEND2MODE(x) (x)
#define LIRC_MODE2REC(x) ((x) << 16)
#define LIRC_REC2MODE(x) ((x) >> 16)

#define LIRC_MODE_RAW                  0x00000001
#define LIRC_MODE_PULSE                0x00000002
#define LIRC_MODE_MODE2                0x00000004
#define LIRC_MODE_CODE                 0x00000008
#define LIRC_MODE_LIRCCODE             0x00000010
#define LIRC_MODE_STRING               0x00000020


#define LIRC_CAN_SEND_RAW              LIRC_MODE2SEND(LIRC_MODE_RAW)
#define LIRC_CAN_SEND_PULSE            LIRC_MODE2SEND(LIRC_MODE_PULSE)
#define LIRC_CAN_SEND_MODE2            LIRC_MODE2SEND(LIRC_MODE_MODE2)
#define LIRC_CAN_SEND_CODE             LIRC_MODE2SEND(LIRC_MODE_CODE)
#define LIRC_CAN_SEND_LIRCCODE         LIRC_MODE2SEND(LIRC_MODE_LIRCCODE)
#define LIRC_CAN_SEND_STRING           LIRC_MODE2SEND(LIRC_MODE_STRING)

#define LIRC_CAN_SEND_MASK             0x0000003f

#define LIRC_CAN_SET_SEND_CARRIER      0x00000100
#define LIRC_CAN_SET_SEND_DUTY_CYCLE   0x00000200

#define LIRC_CAN_REC_RAW               LIRC_MODE2REC(LIRC_MODE_RAW)
#define LIRC_CAN_REC_PULSE             LIRC_MODE2REC(LIRC_MODE_PULSE)
#define LIRC_CAN_REC_MODE2             LIRC_MODE2REC(LIRC_MODE_MODE2)
#define LIRC_CAN_REC_CODE              LIRC_MODE2REC(LIRC_MODE_CODE)
#define LIRC_CAN_REC_LIRCCODE          LIRC_MODE2REC(LIRC_MODE_LIRCCODE)
#define LIRC_CAN_REC_STRING            LIRC_MODE2REC(LIRC_MODE_STRING)

#define LIRC_CAN_REC_MASK              LIRC_MODE2REC(LIRC_CAN_SEND_MASK)

#define LIRC_CAN_SET_REC_CARRIER       (LIRC_CAN_SET_SEND_CARRIER << 16)
#define LIRC_CAN_SET_REC_DUTY_CYCLE    (LIRC_CAN_SET_SEND_DUTY_CYCLE << 16)

#define LIRC_CAN_SET_REC_DUTY_CYCLE_RANGE 0x40000000
#define LIRC_CAN_SET_REC_CARRIER_RANGE    0x80000000


#define LIRC_CAN_SEND(x) ((x)&LIRC_CAN_SEND_MASK)
#define LIRC_CAN_REC(x) ((x)&LIRC_CAN_REC_MASK)

/*
 * IOCTL commands for lirc driver
 */

#define LIRC_GET_FEATURES              _IOR('i', 0x00000000, __u32)

#define LIRC_GET_SEND_MODE             _IOR('i', 0x00000001, __u32)
#define LIRC_GET_REC_MODE              _IOR('i', 0x00000002, __u32)
#define LIRC_GET_SEND_CARRIER          _IOR('i', 0x00000003, __u32)
#define LIRC_GET_REC_CARRIER           _IOR('i', 0x00000004, __u32)
#define LIRC_GET_SEND_DUTY_CYCLE       _IOR('i', 0x00000005, __u32)
#define LIRC_GET_REC_DUTY_CYCLE        _IOR('i', 0x00000006, __u32)

/* code length in bits, currently only for LIRC_MODE_LIRCCODE */
#define LIRC_GET_LENGTH                _IOR('i', 0x0000000f, __u32)

#define LIRC_SET_SEND_MODE             _IOW('i', 0x00000011, __u32)
#define LIRC_SET_REC_MODE              _IOW('i', 0x00000012, __u32)
/* Note: these can reset the according pulse_width */
#define LIRC_SET_SEND_CARRIER          _IOW('i', 0x00000013, __u32)
#define LIRC_SET_REC_CARRIER           _IOW('i', 0x00000014, __u32)
#define LIRC_SET_SEND_DUTY_CYCLE       _IOW('i', 0x00000015, __u32)
#define LIRC_SET_REC_DUTY_CYCLE        _IOW('i', 0x00000016, __u32)

/* to set a range use
   LIRC_SET_REC_DUTY_CYCLE_RANGE/LIRC_SET_REC_CARRIER_RANGE with the
   lower bound first and later
   LIRC_SET_REC_DUTY_CYCLE/LIRC_SET_REC_CARRIER with the upper bound */

#define LIRC_SET_REC_DUTY_CYCLE_RANGE  _IOW('i', 0x0000001e, __u32)
#define LIRC_SET_REC_CARRIER_RANGE     _IOW('i', 0x0000001f, __u32)


#define PL1061_CIR_MODULE_NAME  "consumer_ir"
#define CIR_MINOR 64

#define INCBUF(x,mod) (((x)+1) & ((mod) - 1))
#define CIRBUF_SIZE  256

static int invert = 0;
static int gpio = 9;

struct pl1061_cir_device {
	unsigned int		head, tail;        /* Position in the event buffer */
	//struct fasync_struct	*async_queue;      /* Asynchronous notification    */
	wait_queue_head_t	waitq;             /* Wait queue for reading       */
	struct semaphore	lock;              /* Mutex for reading            */
	unsigned int		usage_count;       /* Increment on each open       */
	unsigned int		total;             /* Total events                 */
	unsigned int		processed;
	unsigned int		dropped;
	lirc_t			buf[CIRBUF_SIZE];
};

struct pl1061_cir_device g_cirdev;

MODULE_AUTHOR("Samson Su");
MODULE_DESCRIPTION("CIR driver for PL1061B");

static void inline writetobuf(lirc_t value)
{
	struct pl1061_cir_device *cdev = &g_cirdev;
	unsigned int nhead;

	// Add this value to the ring buffer.  Discard if we've run out of room ?
	nhead = INCBUF(cdev->head, CIRBUF_SIZE);
	cdev->total++;
	if ( nhead != cdev->tail ) {
		cdev->buf[cdev->head] = value;
		cdev->head = nhead;

		//used to signal the interested process when data arrives
		//if ( cdev->async_queue )
		//	kill_fasync( &cdev->async_queue, SIGIO, POLL_IN );

		wake_up_interruptible( &cdev->waitq );
		cdev->processed++;
	}
	else{
		cdev->dropped++;
		//printk(KERN_WARNING " Buffer overrun\n");
	}
}


static void cir_event_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned long data;
	unsigned short cir_mask = 1 << gpio;
	unsigned long long timestamp;
	static unsigned long long last_timestamp = 0;
	unsigned long long delta;
	int is_space;
	unsigned short pol, low_pol, in;

	pol = readw(reGPIO_KEYREG+2);
	low_pol = pol & cir_mask;
	in = readw(reGPIO_KEY1REG+2) & cir_mask;

	//printk(KERN_INFO "cir interrupt=%x\n", in);

	if (in) {
		if (low_pol) {/* high -> low */
			is_space = invert;
			pol = pol & ~cir_mask;
		} else { /* low -> high */
			is_space = !invert;
			pol = pol | cir_mask;
		}

		timestamp = (((unsigned long long)(readl(rFTSTP_SECOND))) << 32) | readl(rFTSTP_FRACTIONAL);

		delta = timestamp - last_timestamp;


		if ((delta >> 32) > 15ULL){
			data = PULSE_MASK;
		}else {
			data = (unsigned long)(delta >> 12);
		}
		writetobuf(is_space ? data : (data | PULSE_BIT));
		last_timestamp = timestamp;

		writew(pol, reGPIO_KEYREG+2);	/* reverse polarity */
		writew(in, reGPIO_KEY1REG+2);	/* clear interrupt status */

	}

}


#define PL1061_READ_WAIT_FOR_DATA \
   do { \
	if (down_interruptible(&dev->lock))      \
		return -ERESTARTSYS;                 \
	while ( dev->head == dev->tail ) {   \
		up(&dev->lock);                  \
		if ( filp->f_flags & O_NONBLOCK )    \
			return -EAGAIN;              \
		if ( wait_event_interruptible( dev->waitq, (dev->head != dev->tail) ) )  \
			return -ERESTARTSYS;                \
		if ( down_interruptible(&dev->lock))    \
			return -ERESTARTSYS;                \
	} \
   } while (0)


static ssize_t pl1061_cir_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	struct pl1061_cir_device *dev = (struct pl1061_cir_device *) filp->private_data;

	PL1061_READ_WAIT_FOR_DATA;

	if ( copy_to_user((void *)buf, (void *)&dev->buf[dev->tail], sizeof(lirc_t)) ) {
		up(&dev->lock);
		return -EFAULT;
	}

	dev->tail = INCBUF(dev->tail, CIRBUF_SIZE);

	up(&dev->lock);
	return sizeof(lirc_t);
}

/*
static int pl1061_cir_fasync(int fd, struct file *filp, int mode)
{
	struct pl1061_cir_device *dev = (struct pl1061_cir_device *) filp->private_data;

	//fasync_helper is invoked to add files to or remove files from the list of interested process
	//when the FASYNC flag changes for an open file.
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}
*/

//inquire if a device is readable or writable or in some specail state
static unsigned int pl1061_cir_poll( struct file * filp, poll_table *wait )
{
	struct pl1061_cir_device *dev = (struct pl1061_cir_device *) filp->private_data;
	poll_wait(filp, &dev->waitq, wait);
	return (dev->head == dev->tail ? 0 : (POLLIN | POLLRDNORM));
}

static int pl1061_cir_ioctl(struct inode * inode, struct file *filp,
		       unsigned int cmd , unsigned long arg)
{
	int retval = 0;

	unsigned long value = 0;

	if (cmd == LIRC_GET_FEATURES)
		value = LIRC_CAN_REC_MODE2;
	else if (cmd == LIRC_GET_REC_MODE)
		value = LIRC_MODE_MODE2;

	switch (cmd) {
	case LIRC_GET_FEATURES:
	case LIRC_GET_REC_MODE:
		retval = put_user(value, (unsigned long *) arg);
		break;
	case LIRC_SET_REC_MODE:
		retval = get_user(value, (unsigned long *) arg);
		break;
	default:
		retval = -ENOIOCTLCMD;
	}

	if (retval)
		return retval;

	if (cmd == LIRC_SET_REC_MODE) {
		if (value != LIRC_MODE_MODE2)
			retval = -ENOSYS;
	}
	return retval;
}

static int pl1061_cir_release(struct inode * inode, struct file * filp)
{
	struct pl1061_cir_device *dev = (struct pl1061_cir_device *) filp->private_data;

	unsigned short cir_mask = 1 << gpio;

	dev->usage_count--;
        //printk(KERN_INFO " dev-usage_count:%d\n", dev->usage_count);

	//filp->f_op->fasync( -1, filp, 0 );  /* Remove ourselves from the async list */
	MOD_DEC_USE_COUNT;

	//disable interrupt
	writew(readw(reGPIO_KEYREG) & ~cir_mask, reGPIO_KEYREG);
	return 0;
}


static int pl1061_cir_open( struct inode * inode, struct file * filp)
{

	struct pl1061_cir_device *dev = (struct pl1061_cir_device *) filp->private_data;

	unsigned int oe_reg, od_reg;
	unsigned short cir_mask = 1 << gpio;

	PDEBUG("\n");
	//printk(KERN_INFO "==>cir61: invert=%d, gpio=%d\n", invert, gpio);

	if ( dev->usage_count++ == 0 ) {
		dev->tail = dev->head;  // We're the first open - clear the buffer
	}
	//printk(KERN_INFO " dev-usage_count:%d\n", dev->usage_count);
	MOD_INC_USE_COUNT;

	writew(0, reGPIO_KEY0REG+2);

	//writel(0x20000000, reTCFG0);
        //writel(0x00000000, reTCFG1);

	/* config cir pin as input pin */
	od_reg = readw(reGPIO_OD) & ~(cir_mask);
	writew(od_reg, reGPIO_OD);

	/* pull up cir pin */
	writew(readw(reGPIO_PF0) | cir_mask, reGPIO_PF0);

	/* enable schmitt trigger */
	writew(readw(reGPIO_PF1) | cir_mask, reGPIO_PF1);

	/* enable cir pin interrupt mask */
	writew(readw(reGPIO_KEY1REG) | cir_mask, reGPIO_KEY1REG);

	/* enable cir pin (keypad enable)  */
	writew(readw(reGPIO_KEYREG) | cir_mask, reGPIO_KEYREG);

	/* config polarity as low active */
	writew(readw(reGPIO_KEYREG+2) | cir_mask, reGPIO_KEYREG+2);
#ifdef CONFIG_PL1063
	writew(RSTN_PWM | RSTN_KEY | RSTN_INTX, reGPIO_KEY0REG+2);
#else
	writew(RSTN_PWM | RSTN_KEY, reGPIO_KEY0REG+2);
#endif

	/* enable cir gpio */
	oe_reg = readw(reGPIO_OE) | cir_mask;
	writew(oe_reg, reGPIO_OE);

	return 0;
}


struct file_operations cir_fops = {
	read:           pl1061_cir_read,
	ioctl:          pl1061_cir_ioctl,
	poll:           pl1061_cir_poll,
	open:           pl1061_cir_open,
	release:        pl1061_cir_release,
};

static int pl1061_cir_open_generic(struct inode * inode, struct file * filp)
{
        unsigned int minor = MINOR( inode->i_rdev );   /* Extract the minor number */
	int result;

	PDEBUG("\n");

	if ( minor > 0 ) {
		printk(KERN_WARNING " bad minor = %d\n", minor );
		return -ENODEV;
	}

        if ( !filp->private_data ) {
                switch (minor) {
                case CIR_MINOR:
                        filp->private_data = &g_cirdev;
		        filp->f_op = &cir_fops;
                        break;
                }
        }

        result = filp->f_op->open( inode, filp );
        if ( !result )
                 return result;
        return 0;
}

struct file_operations cir_generic_fops = {
	open:     pl1061_cir_open_generic
};


static int pl1061_cir_proc_read(char *page, char **start, off_t off,
			      int count, int *eof, void *data)
{
	char *p = page;
	int len;

	p += sprintf(p, "                 Total  Processed Dropped\n");
	p += sprintf(p, "CIR     : %8d %8d %8d\n", g_cirdev.total, g_cirdev.processed, g_cirdev.dropped);

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

/***********************************************************************************/
/*       Initialization                                                            */
/***********************************************************************************/

static devfs_handle_t devfs_dev;
static int gMajor;		/* Dynamic major for now */

void pl1061_cir_init_device(struct pl1061_cir_device *dev)
{

	dev->head = 0;
	dev->tail = 0;

	init_waitqueue_head( &dev->waitq );
	init_MUTEX( &dev->lock );//the semaphore should be initialized to a value of 1, which means
	                         //that the semaphore is available
	//dev->async_queue = NULL;
}


int __init pl1061_cir_init_module(void)
{
        printk(KERN_INFO "cir61: init cunsumer IR module\n");
        gMajor = devfs_register_chrdev(0, PL1061_CIR_MODULE_NAME, &cir_generic_fops);
        if (gMajor < 0) {
                printk(KERN_WARNING "cir61: CIR module can't get major number\n");
                return gMajor;
        }
	//LIRC package use device "/dev/lirc"
	devfs_dev = devfs_register( NULL, "lirc", DEVFS_FL_DEFAULT,
				       gMajor, CIR_MINOR,
				       S_IFCHR | S_IRUSR | S_IWUSR,
				       &cir_fops, &g_cirdev );

	pl1061_cir_init_device(&g_cirdev);

	//connect pl1061_cir_proc_read to an entry in the /proc hierarchy, example: /proc/lirc
	create_proc_read_entry("lirc", 0, NULL, pl1061_cir_proc_read, NULL);

	if (request_irq(IRQ_PL_PWM, cir_event_isr, SA_SHIRQ,
                             "cir", &g_cirdev)){
                printk(KERN_CRIT "cir61: CIR module allocate IRQ failed !\n");
                return -ENOMEM;
        }

	return 0;
}

void pl1061_cir_cleanup_module(void)
{
	printk(KERN_INFO "cir61: shutting down CIR module\n");

	free_irq(IRQ_PL_PWM, &g_cirdev);

	remove_proc_entry("lirc", NULL);

        //deivce removal
	devfs_unregister(devfs_dev);

        //release the major number
	devfs_unregister_chrdev(gMajor, PL1061_CIR_MODULE_NAME);
}

/*
 * Grammar : pl1061.cir={invert;}<gpio>;
 *       <gpio> := decimal integer(0~15), config which GPIO pin will receive cir signal, but
 *                could not conflict with GPIO pins used by keypad.
 *       invert := invert remote control signal
 *
 * Example : pl1061.cir=invert;9
 */
int __init cir61_setup(char *options)
{
    if (strncmp(options, "invert;", 7) == 0) {
        options += 7;
        invert = 1;
    }

    if (strncmp(options, "gpio:", 5) == 0) {
        options += 5;
        gpio = simple_strtoul(options, &options, 10);
    }

    return 1;
}
__setup("pl1061.cir=", cir61_setup);

module_init(pl1061_cir_init_module);
module_exit(pl1061_cir_cleanup_module);

