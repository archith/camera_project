/*
 * File:
 *	linux/drivers/pl1061/rtc.c
 *
 * Copyright:
 *	Copyright (C) 2003 Prolific Technology Inc.
 *
 * Description:
 *	Real Time Clock interface for Linux
 *
 * History:
 * 	03/31/03: modified from linux/drivers/rtc.c
 * 		  David Kao <david-kao@prolific.com.tw>
 */
/*
 *	Real Time Clock interface for Linux
 *
 *	Copyright (C) 1996 Paul Gortmaker
 *
 *	This driver allows use of the real time clock (built into
 *	nearly all computers) from user space. It exports the /dev/rtc
 *	interface supporting various ioctl() and also the
 *	/proc/driver/rtc pseudo-file for status information.
 *
 *	The ioctls can be used to set the interrupt behaviour and
 *	generation rate from the RTC via IRQ 8. Then the /dev/rtc
 *	interface can be used to make use of these timer interrupts,
 *	be they interval or alarm based.
 *
 *	The /dev/rtc interface will block on reads until an interrupt
 *	has been received. If a RTC interrupt has already happened,
 *	it will output an unsigned long and then block. The output value
 *	contains the interrupt status in the low byte and the number of
 *	interrupts since the last read in the remaining high bytes. The
 *	/dev/rtc interface can also be used with the select(2) call.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *	Based on other minimal char device drivers, like Alan's
 *	watchdog, Ted's random, etc. etc.
 *
 *	1.07	Paul Gortmaker.
 *	1.08	Miquel van Smoorenburg: disallow certain things on the
 *		DEC Alpha as the CMOS clock is also used for other things.
 *	1.09	Nikita Schmidt: epoch support and some Alpha cleanup.
 *	1.09a	Pete Zaitcev: Sun SPARC
 *	1.09b	Jeff Garzik: Modularize, init cleanup
 *	1.09c	Jeff Garzik: SMP cleanup
 *	1.10    Paul Barton-Davis: add support for async I/O
 *	1.10a	Andrea Arcangeli: Alpha updates
 *	1.10b	Andrew Morton: SMP lock fix
 *	1.10c	Cesar Barros: SMP locking fixes and cleanup
 *	1.10d	Paul Gortmaker: delete paranoia check in rtc_exit
 *	1.10e	Maciej W. Rozycki: Handle DECstation's year weirdness.
 */

/*
 *	Note that *all* calls to CMOS_READ and CMOS_WRITE are done with
 *	interrupts disabled. Due to the index-port/data-port (0x70/0x71)
 *	design of the RTC, we don't want two different things trying to
 *	get to it at once. (e.g. the periodic 11 min sync from time.c vs.
 *	this driver.)
 */

static char *rtc_name = "Prolific Real-Time Clock Driver";
static char *rtc_version = "1.0.0";
static char *rtc_revdate = "2003-04-02";

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/rtc.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/hardware.h>

#if 1
#include <linux/devfs_fs_kernel.h>
#define  _DEVFS_
#define  RTC_NUMBER 223
#endif
/* RTC register define */
#define rTICKER_PERIOD		( rTIMER_BASE + 0x00000000 )
#define rTICKER_START		( rTIMER_BASE + 0x00000001 )
#define rTICKER_CLEAR		( rTIMER_BASE + 0x00000002 )
#define rWATCH_DOG_CLEAR	( rTIMER_BASE + 0x00000003 )
#define rRTC			    ( rTIMER_BASE + 0x00000004 )
#define rALM_INTVECT		( rTIMER_BASE + 0x00000008 )
#define rALM_INTCLR_0		( rTIMER_BASE + 0x00000009 )
#define rALM_INTCLR_1		( rTIMER_BASE + 0x0000000A )
#define rALM_INTCLR_2		( rTIMER_BASE + 0x0000000B )
#define rALM_INTCLR_3		( rTIMER_BASE + 0x0000000C )
#define rPWRSW_INTCLR		( rTIMER_BASE + 0x0000000D )
#define rFALM_INTCLR		( rTIMER_BASE + 0x0000000E )
#define rFALM_INTDIS		( rTIMER_BASE + 0x0000000F )
#define rALM_0			    ( rTIMER_BASE + 0x00000010 )
#define rALM_1			    ( rTIMER_BASE + 0x00000014 )
#define rALM_2			    ( rTIMER_BASE + 0x00000018 )
#define rALM_3			    ( rTIMER_BASE + 0x0000001C )
#define rALM_FINE		    ( rTIMER_BASE + 0x00000020 )
#define rFTIMESTAMP		    ( rTIMER_BASE + 0x00000028 )

#ifdef _DEVFS_
static devfs_handle_t rtc_handle;
#endif

/*
 * structure definitions
 */

/*
 * the entry of the alarm list for all registered user-level processes
 */
struct rtc_ualarm {
	struct list_head u_list;
	long		 u_time;	/* the time to set alarm */
	long		 u_interval;	/* the interval to raise alarm */
};
typedef struct rtc_ualarm *rtc_ualarm_t;
#define RTC_UALARM_NULL		((rtc_ualarm_t)0)


/*
 * external variables
 */
extern spinlock_t rtc_lock;


/*
 * configuration
 */
static int rtc_irq = IRQ_PL_ALARM;


/*
 *	We sponge a minor off of the misc major. No need slurping
 *	up another valuable major dev number for this. If you add
 *	an ioctl, make sure you don't conflict with SPARC's RTC
 *	ioctls.
 */
static struct fasync_struct *rtc_async_queue;

static DECLARE_WAIT_QUEUE_HEAD(rtc_wait);

/*
 *	Bits in rtc_status. (6 bits of room for future expansion)
 */
#define RTC_IS_OPEN		0x01	/* means /dev/rtc is in use	*/
#define RTC_TIMER_ON		0x02	/* missed irq timer active	*/

/*
 * rtc_status is never changed by rtc_interrupt, and ioctl/open/close is
 * protected by the big kernel lock. However, ioctl can still disable the timer
 * in rtc_status and then with del_timer after the interrupt has read
 * rtc_status but before mod_timer is called, which would then reenable the
 * timer (but you would need to have an awful timing before you'd trip on it)
 */
static unsigned long rtc_status = 0;	/* bitmapped status byte.	*/
static unsigned long rtc_irq_data = 0;	/* our output to the world	*/

/*
 *	If this driver ever becomes modularised, it will be really nice
 *	to make the epoch retain its value across module reload...
 */

static unsigned epoch = 1900;	/* year corresponding to 0x00	*/

static const unsigned char days_in_mo[] = {
	0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 * alarm for the process timer and the kernel timer
 */
static LIST_HEAD(rtc_ualarm_list);


/*
 * macros
 */
#define rtc_alarm_cli()		disable_irq(rtc_irq)
#define rtc_alarm_sti()		enable_irq(rtc_irq)


/*
 *  forward references
 */
static ssize_t	    rtc_read(struct file *file, char *buf,
			     size_t count, loff_t *ppos);
static int 	    rtc_ioctl(struct inode *inode, struct file *file,
		     	      unsigned int cmd, unsigned long arg);
static unsigned int rtc_poll(struct file *file, poll_table *wait);
static int	    rtc_read_proc(char *page, char **start, off_t off,
				  int count, int *eof, void *data);

static void rtc_get_time(struct rtc_time *rtc_tm);
static int  rtc_set_time(struct rtc_time *rtc_tm);
static void rtc_get_alm_time(struct rtc_time *alm_tm);
static int  rtc_set_alm_time(struct rtc_time *alm_tm);

#if 0
static __inline__ void rtc_ualarm_add(long nsecs);
static __inline__ void rtc_ualarm_adj(unsigned long old, unsigned long new);
static __inline__ void rtc_ualarm_next(void);
static		  void rtc_ualarm_dump(void);
#endif

static __inline__ void rtc_doset(unsigned int *addr, time_t tv_sec);
static 		  void to_tm(unsigned long tim, struct rtc_time * tm);




#ifdef  CONFIG_FALARM
// 2003/09/19 Pax Tsai.
extern int __init falarm_init(void);
static int falarm_used=0;
static unsigned long falarm_irqflags=0;
static void (*falarm_irq_handler)(int, void *, struct pt_regs *)=NULL;


#define MASK_FALARM_IRQ     writeb(readb(rFALM_INTDIS)|1, rFALM_INTDIS)
#define UNMASK_FALARM_IRQ   writeb(readb(rFALM_INTDIS)&0xFE, rFALM_INTDIS)


static void rtc_do_FALARM_IRQ(struct pt_regs *regs)
{
    int cpu, sti;


    cpu = smp_processor_id();
    barrier();

    if (falarm_irq_handler) {
        if ((sti = !(falarm_irqflags & SA_INTERRUPT))) {
            MASK_FALARM_IRQ;
            __sti();
        }

        falarm_irq_handler(rtc_irq, NULL, regs);

        if (sti) {
            __cli();
            UNMASK_FALARM_IRQ;
        }
    }

    barrier();

    /* unmasking and bottom half handling is done magically for us. */
}


int rtc_request_irq(unsigned int irq,
        void (*handler)(int, void *, struct pt_regs *),
        unsigned long irqflags, const char *devname, void *dev_id)
{
    extern char _stext, _end;

    if (!falarm_used)
        return -EIO;

    if (irq != rtc_irq)
        return -EINVAL;

    if (!handler)
        return -EINVAL;

    if (! ( (char *)handler >= &_stext && (char *)handler < &_end))
        return -EINVAL;

    if (((unsigned long)(handler) & 0x03) != 0)
        return -EINVAL;

    falarm_irq_handler = handler;
    falarm_irqflags = irqflags;

    // Others are useless.

    enable_irq(rtc_irq);

    return 0;
}


void rtc_free_irq(unsigned int irq, void *dev_id)
{
    if (!falarm_used)
        return -EIO;

    disable_irq(rtc_irq);

    falarm_irq_handler = NULL;
    falarm_irqflags = 0;
}


#endif  // CONFIG_FALARM



/*
 * functions
 */

/*
 *	A very tiny interrupt handler. It runs with SA_INTERRUPT set,
 *	but there is possibility of conflicting with the set_rtc_mmss()
 *	call (the rtc irq and the timer irq can easily run at the same
 *	time in two different CPUs). So we need to serializes
 *	accesses to the chip with the rtc_lock spinlock that each
 *	architecture should implement in the timer code.
 *	(See ./arch/XXXX/kernel/time.c for the set_rtc_mmss() function.)
 */

static void
rtc_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
#define ALM_INTVECT_MASK	0x1F
#define ALM_INTVECT_0		0x00
#define ALM_INTVECT_1		0x01
#define ALM_INTVECT_2		0x02
#define ALM_INTVECT_3		0x03
#define ALM_INTVECT_FINE	0x10

	u_char alm_vect;

	alm_vect = readb(rALM_INTVECT);
/*
	printk("%s: alm_vect = %x\n", __FUNCTION__, alm_vect);
*/

	switch (alm_vect) {
    case ALM_INTVECT_0:
		printk("%s: (RTC, ALM) = (0x%x, 0x%x)\n", __FUNCTION__,
			readw(rRTC), readw(rALM_0));
            writeb(0xFF, rALM_INTCLR_0);
		break;
	case ALM_INTVECT_1:
		printk("%s: (RTC, ALM) = (0x%x, 0x%x)\n", __FUNCTION__,
            readw(rRTC), readw(rALM_1));
#if 0
		rtc_ualarm_next();
#endif
        writeb(0xff, rALM_INTCLR_1);
		break;
	case ALM_INTVECT_2:
        writeb(0xff, rALM_INTCLR_2);
		break;
    case ALM_INTVECT_3:
        writeb(0xff, rALM_INTCLR_3);
		break;

    case ALM_INTVECT_FINE:
#ifdef  CONFIG_FALARM
        rtc_do_FALARM_IRQ(regs);
#else
        writeb(0xff, rFALM_INTCLR);
#endif
        break;
	}

	/*
	 *	Can be an alarm interrupt, update complete interrupt,
	 *	or a periodic interrupt. We store the status in the
	 *	low byte and the number of interrupts received since
	 *	the last read in the remainder of rtc_irq_data.
	 */
	spin_lock (&rtc_lock);
	rtc_irq_data++;
	spin_unlock (&rtc_lock);

	/* Now do the rest of the actions */
	wake_up_interruptible(&rtc_wait);

	kill_fasync(&rtc_async_queue, SIGIO, POLL_IN);
} /* rtc_interrupt() */

/*
 *	Now all the various file operations that we export.
 */
static long long
rtc_llseek(struct file * file, long long offset, int origin)
{
	return -ESPIPE;
} /* rtc_llseek() */

static ssize_t
rtc_read(struct file *file, char *buf,
	 size_t count, loff_t *ppos)
{
	DECLARE_WAITQUEUE(wait, current);
	unsigned long data;
	ssize_t retval;

	if (count < sizeof(unsigned long)) return -EINVAL;

	add_wait_queue(&rtc_wait, &wait);

	current->state = TASK_INTERRUPTIBLE;

	do {
		/* First make it right. Then make it fast. Putting this whole
		 * block within the parentheses of a while would be too
		 * confusing. And no, xchg() is not the answer. */
		spin_lock_irq (&rtc_lock);
		data = rtc_irq_data;
		rtc_irq_data = 0;
		spin_unlock_irq (&rtc_lock);

		if (data != 0) break;

		if (file->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto out;
		}
		if (signal_pending(current)) {
			retval = -ERESTARTSYS;
			goto out;
		}
		schedule();
	} while (1);

	retval = put_user(data, (unsigned long *)buf);
	if (!retval)
		retval = sizeof(unsigned long);
 out:
	current->state = TASK_RUNNING;
	remove_wait_queue(&rtc_wait, &wait);

	return retval;
} /* rtc_read() */

static int
rtc_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	  unsigned long arg)
{
	struct rtc_time wtime;

	switch (cmd) {
	case RTC_AIE_OFF:	/* Mask alarm int. enab. bit	*/
		disable_irq(rtc_irq);
#ifdef CONFIG_PM
		pm_disable_irq(rtc_irq);
#endif /* CONFIG_PM */
		return 0;
	case RTC_AIE_ON:	/* Allow alarm interrupts.	*/
		enable_irq(rtc_irq);
#ifdef CONFIG_PM
		pm_enable_irq(rtc_irq);
#endif /* CONFIG_PM */
		return 0;
	case RTC_PIE_OFF:	/* Mask periodic int. enab. bit	*/
		return -EINVAL;
	case RTC_PIE_ON:	/* Allow periodic ints		*/
		return -EINVAL;
	case RTC_UIE_OFF:	/* Mask ints from RTC updates.	*/
		return -EINVAL;
	case RTC_UIE_ON:	/* Allow ints for RTC updates.	*/
		return -EINVAL;
	case RTC_ALM_READ:	/* Read the present alarm time */
		/*
		 * This returns a struct rtc_time. Reading >= 0xc0
		 * means "don't care" or "match all". Only the tm_hour,
		 * tm_min, and tm_sec values are filled in.
		 */

		rtc_get_alm_time(&wtime);
		break;
	case RTC_ALM_SET:	/* Store a time into the alarm */
	{
		struct rtc_time alm_tm;

		if (copy_from_user(&alm_tm, (struct rtc_time*)arg,
				   sizeof(struct rtc_time)))
			return -EFAULT;

		return rtc_set_alm_time(&alm_tm);
	}
	case RTC_RD_TIME:	/* Read the time/date from RTC	*/
		rtc_get_time(&wtime);
		break;
	case RTC_SET_TIME:	/* Set the RTC */
	{
		struct rtc_time rtc_tm;

		if (!capable(CAP_SYS_TIME)) return -EACCES;

		if (copy_from_user(&rtc_tm, (struct rtc_time*)arg,
				   sizeof(struct rtc_time)))
			return -EFAULT;

		return rtc_set_time(&rtc_tm);
	}
	case RTC_IRQP_READ:	/* Read the periodic IRQ rate.	*/
		return -EINVAL;
	case RTC_IRQP_SET:	/* Set periodic IRQ rate.	*/
		return -EINVAL;
	case RTC_EPOCH_READ:	/* Read the epoch.	*/
		return put_user (epoch, (unsigned long *)arg);
	case RTC_EPOCH_SET:	/* Set the epoch.	*/
		/*
		 * There were no RTC clocks before 1900.
		 */
		if (arg < 1900) return -EINVAL;

		if (!capable(CAP_SYS_TIME)) return -EACCES;

		epoch = arg;
		return 0;
	default:
		return -EINVAL;
	}

	return copy_to_user((void *)arg, &wtime, sizeof wtime) ? -EFAULT : 0;
} /* rtc_iotcl() */

/*
 *	We enforce only one user at a time here with the open/close.
 *	Also clear the previous interrupt data on an open, and clean
 *	up things on a close.
 */

/* We use rtc_lock to protect against concurrent opens. So the BKL is not
 * needed here. Or anywhere else in this driver. */
static int
rtc_open(struct inode *inode, struct file *file)
{
	spin_lock_irq (&rtc_lock);

	if (rtc_status & RTC_IS_OPEN) goto out_busy;

	rtc_status |= RTC_IS_OPEN;

	rtc_irq_data = 0;
	spin_unlock_irq (&rtc_lock);
	return 0;

out_busy:
	spin_unlock_irq (&rtc_lock);
	return -EBUSY;
} /* rtc_open() */

static int
rtc_fasync(int fd, struct file *filp, int on)
{
	return fasync_helper(fd, filp, on, &rtc_async_queue);
} /* rtc_fasync() */

static int
rtc_release(struct inode *inode, struct file *file)
{
	/*
	 * Turn off all interrupts once the device is no longer
	 * in use, and clear the data.
	 */

	if (file->f_flags & FASYNC) {
		rtc_fasync (-1, file, 0);
	}

	spin_lock_irq (&rtc_lock);
	rtc_irq_data = 0;
	spin_unlock_irq (&rtc_lock);

	/* No need for locking -- nobody else can do anything until this rmw is
	 * committed, and no timer is running. */
	rtc_status &= ~RTC_IS_OPEN;

	return 0;
} /* rtc_release() */

/* Called without the kernel lock - fine */
static unsigned int
rtc_poll(struct file *file, poll_table *wait)
{
	unsigned long l;

	poll_wait(file, &rtc_wait, wait);

	spin_lock_irq(&rtc_lock);
	l = rtc_irq_data;
	spin_unlock_irq(&rtc_lock);

	if (l != 0) return POLLIN | POLLRDNORM;

	return 0;
} /* rtc_poll() */

/*
 *	The various file operations we support.
 */
static struct file_operations rtc_fops = {
	owner:		THIS_MODULE,
	llseek:		rtc_llseek,
	read:		rtc_read,
	poll:		rtc_poll,
	ioctl:		rtc_ioctl,
	open:		rtc_open,
	release:	rtc_release,
	fasync:		rtc_fasync,
};

static struct miscdevice rtc_dev= {
	RTC_MINOR,
	"rtc",
	&rtc_fops
};


static int __init
rtc_init(void)
{
	/*
	 *  initialization of all used ALARM registers
	 */
	rtc_doset((unsigned int *)rALM_0, 0xFFFFFFFF);
	rtc_doset((unsigned int *)rALM_1, 0xFFFFFFFF);


#ifdef  CONFIG_FALARM
    if (falarm_init() == 0)
        falarm_used = 1;
#endif

	/*
	 * init alarm
	 */
	if (request_irq(rtc_irq, rtc_interrupt, SA_INTERRUPT, "rtc", NULL)) {
		/* Yeah right, seeing as irq 8 doesn't even hit the bus. */
		printk(KERN_ERR "rtc: IRQ %d is not free.\n", rtc_irq);
		return -EIO;
	}

	disable_irq(rtc_irq);

#ifdef _DEVFS_ 
  	if (devfs_register_chrdev(RTC_NUMBER, "rtc", &rtc_fops)) {
        	printk(KERN_NOTICE "Can't allocate major number %d for ADDR Devices\n",
                RTC_NUMBER);
        	return -EAGAIN;
    	}

    	rtc_handle = devfs_register(NULL, "rtc", DEVFS_FL_DEFAULT,
                        RTC_NUMBER, 0,
                        S_IFCHR | S_IRUGO | S_IWUGO,
                        &rtc_fops, NULL);

#else
	misc_register(&rtc_dev);
#endif
	create_proc_read_entry("driver/rtc", 0, 0, rtc_read_proc, NULL);

	printk(KERN_INFO "%s version %s (%s)\n",
			 rtc_name, rtc_version, rtc_revdate);

	return 0;
} /* rtc_init() */

static void __exit
rtc_exit(void)
{
	remove_proc_entry("driver/rtc", NULL);

#ifdef _DEVFS_
	devfs_unregister(rtc_handle);
    	devfs_unregister_chrdev( RTC_NUMBER, "rtc");
#else
	misc_deregister(&rtc_dev);
#endif

#if CONFIG_PM
	pm_disable_irq(rtc_irq);
#endif
	free_irq(rtc_irq, NULL);
} /* rtc_exit() */

__initcall(rtc_init);
__exitcall(rtc_exit);
EXPORT_NO_SYMBOLS;


/*
 *	Info exported via "/proc/driver/rtc".
 */

static int
rtc_proc_output(char *buf)
{
#define YN(bit) ((ctrl & bit) ? "yes" : "no")
#define NY(bit) ((ctrl & bit) ? "no" : "yes")
	char *p;
	struct rtc_time tm;

	p = buf;

	rtc_get_time(&tm);

	/*
	 * There is no way to tell if the luser has the RTC set for local
	 * time or for Universal Standard Time (GMT). Probably local though.
	 */
	p += sprintf(p,
		     "rtc_time\t: %02d:%02d:%02d\n"
		     "rtc_date\t: %04d-%02d-%02d\n"
	 	     "rtc_epoch\t: %04u\n",
		     tm.tm_hour, tm.tm_min, tm.tm_sec,
		     tm.tm_year + epoch, tm.tm_mon + 1, tm.tm_mday, epoch);

	rtc_get_alm_time(&tm);

	/*
	 * We implicitly assume 24hr mode here. Alarm values >= 0xc0 will
	 * match any value for that particular field. Values that are
	 * greater than a valid time, but less than 0xc0 shouldn't appear.
	 */
	p += sprintf(p, "alarm\t\t: ");
	if (tm.tm_hour <= 24)
		p += sprintf(p, "%02d:", tm.tm_hour);
	else
		p += sprintf(p, "**:");

	if (tm.tm_min <= 59)
		p += sprintf(p, "%02d:", tm.tm_min);
	else
		p += sprintf(p, "**:");

	if (tm.tm_sec <= 59)
		p += sprintf(p, "%02d\n", tm.tm_sec);
	else
		p += sprintf(p, "**\n");

#if 0
	/*
	 * rtc_ualarm_dump()
	 */
	p += sprintf(p, "ualarm\t\t: ");
	{
	    struct list_head  *head, *pos;
	    rtc_ualarm_t      rua;
	    int		      i = 0;

	    rtc_alarm_cli();

	    head = &rtc_ualarm_list;
	    list_for_each(pos, head) {
		rua = list_entry(pos, struct rtc_ualarm, u_list);
		if (i > 0) p += sprintf(p, "\t\t  ");
		p += sprintf(p, "[%02d]: (0x%x, %d) = 0x%x\n",
				i++, (unsigned int)rua->u_time,
				(unsigned int)rua->u_interval,
				(unsigned int)(rua->u_time + rua->u_interval));
    	    }

	    rtc_alarm_sti();

	    if (i == 0) p += sprintf(p, "(null)\n");
	} /* rtc_ualarm_dump() */
#endif

	return  p - buf;
#undef YN
#undef NY
} /* read_proc_output() */

static int
rtc_read_proc(char *page, char **start, off_t off,
	      int count, int *eof, void *data)
{
        int len = rtc_proc_output(page);

        if (len <= off+count) *eof = 1;

        *start = page + off;
        len -= off;
        if (len > count) len = count;
        if (len < 0) len = 0;

        return len;
} /* rtc_read_proc() */


/*
 * helper routines for the RTC ioctl() routine
 */

static void
rtc_get_time(struct rtc_time *rtc_tm)
{
#if 0
	unsigned long nowtime = readw(rRTC);
#else
	unsigned long nowtime = readl(rRTC);
#endif

	to_tm(nowtime, rtc_tm);

	rtc_tm->tm_year -= epoch;
	rtc_tm->tm_mon  -= 1;
} /* rtc_get_time() */

static int
rtc_set_time(struct rtc_time *rtc_tm)
{
	unsigned long nowtime;
	unsigned char mon, day, hrs, min, sec, leap_yr;
	unsigned int  yrs;

	yrs = rtc_tm->tm_year + epoch;
	mon = rtc_tm->tm_mon + 1;   /* tm_mon starts at zero */
	day = rtc_tm->tm_mday;
	hrs = rtc_tm->tm_hour;
	min = rtc_tm->tm_min;
	sec = rtc_tm->tm_sec;

	if (yrs < 1970) return -EINVAL;

	leap_yr = ((!(yrs % 4) && (yrs % 100)) || !(yrs % 400));

	if ((mon > 12) || (day == 0)) return -EINVAL;

	if (day > (days_in_mo[mon] + ((mon == 2) && leap_yr))) return -EINVAL;

	if ((hrs >= 24) || (min >= 60) || (sec >= 60)) return -EINVAL;

	if ((yrs - epoch) > 255)    /* They are unsigned */
		return -EINVAL;

        nowtime = mktime(yrs, mon, day, hrs, min, sec);

	spin_lock_irq(&rtc_lock);
	rtc_doset((unsigned int *)rRTC, nowtime);
	spin_unlock_irq(&rtc_lock);

	return 0;
} /* rtc_set_time() */

static void
rtc_get_alm_time(struct rtc_time *alm_tm)
{
	unsigned long almtime;

	/*
	 * Only the values that we read from the RTC are set. That
	 * means only tm_hour, tm_min, and tm_sec.
	 */
	spin_lock_irq(&rtc_lock);
	almtime = readw(rALM_0);
	spin_unlock_irq(&rtc_lock);

	to_tm(almtime, alm_tm);

	alm_tm->tm_year -= epoch;
	alm_tm->tm_mon  -= 1;
} /* rtc_get_alm_time() */

static int
rtc_set_alm_time(struct rtc_time *alm_tm)
{
	unsigned int almtime;
	unsigned int  mon, day, hrs, min, sec, leap_yr;
	unsigned int  yrs;

	yrs = alm_tm->tm_year + epoch;
	mon = alm_tm->tm_mon + 1;   /* tm_mon starts at zero */
	day = alm_tm->tm_mday;
	hrs = alm_tm->tm_hour;
	min = alm_tm->tm_min;
	sec = alm_tm->tm_sec;

	if (yrs < 1970) return -EINVAL;

	leap_yr = ((!(yrs % 4) && (yrs % 100)) || !(yrs % 400));

	if ((mon > 12) || (day == 0)) return -EINVAL;

	if (day > (days_in_mo[mon] + ((mon == 2) && leap_yr))) return -EINVAL;

	if ((hrs >= 24) || (min >= 60) || (sec >= 60)) return -EINVAL;

	if ((yrs - epoch) > 255)    /* They are unsigned */
		return -EINVAL;

        almtime = mktime(yrs, mon, day, hrs, min, sec);

	spin_lock_irq(&rtc_lock);
	rtc_doset((unsigned int *)rALM_0, almtime);
	spin_unlock_irq(&rtc_lock);

	return 0;
} /* rtc_set_alm_time() */


#if 0

/*
 * alarm routines for user-level processes
 */

int
rtc_setualarm(long nsecs)
{
#define RTC_UALARM_ADHOC	5
    /*
     * if alarm timer is too small,
     * it is not necessary to set hardware alarm
     */
    if (nsecs < RTC_UALARM_ADHOC) return 1;

    rtc_ualarm_add(nsecs);

    return 0;
} /* rtc_setualarm() */

void
rtc_adjualarm(unsigned long old, unsigned long new)
{
    rtc_ualarm_adj(old, new);
} /* rtc_adjualarm() */

/*
 * to add a new registerd alarm
 *
 * three chores to do
 * 1. to delete expired registered alarms if necessary
 * 2. to add the specified alarm if possbile
 * 3. to set the hardware alarm for the lastest alarm if necessary
 */
static __inline__ void
rtc_ualarm_add(long nsecs)
{
    unsigned long     nowtime = readw(rRTC);
    struct list_head  *head, *pos, *ppos;
    rtc_ualarm_t      rua;
    rtc_ualarm_t      new;

   /*
    * 1. to delete expired registered alarms if necessary
    */
    rtc_alarm_cli();

    head = &rtc_ualarm_list;
    ppos = head;
    list_for_each(pos, head) {
	rua = list_entry(pos, struct rtc_ualarm, u_list);

	if (rua->u_time + rua->u_interval > nowtime) break;

	list_del(pos);
	kfree(rua);

	pos = ppos;
    }

    rtc_alarm_sti();

    /*
     * 2. to add the specified alarm if possbile
     */
    new = kmalloc(sizeof(struct rtc_ualarm), GFP_KERNEL);
    if (!new) return;

    new->u_time     = nowtime;
    new->u_interval = nsecs;

    rtc_alarm_cli();

    head = &rtc_ualarm_list;
    ppos = head;
    list_for_each(pos, head) {
	rua = list_entry(pos, struct rtc_ualarm, u_list);

	if (nowtime + nsecs > rua->u_time + rua->u_interval) {
	    ppos = pos;
	}
	else if (nowtime + nsecs == rua->u_time + rua->u_interval) {
	    kfree(new);
	    break;
	}
	else
	    break;
    }
    if (pos == head)
	list_add_tail(&new->u_list, head);
    else
    	__list_add(&new->u_list, ppos, pos);

    /*
     * 3. to set the hardware alarm for the lastest alarm if necessary
     */
    if (new->u_list.prev == head)
	rtc_doset((unsigned int *)rALM_1, nowtime + nsecs);

    rtc_alarm_sti();
} /* rtc_ualarm_add() */

/*
 * to adjust registered alarms due to the change of the system time
 *
 * three chores to do
 * 1. to delete expired registered alarms if necessary
 * 2, to adjust registered alarms according to the old and new system times
 * 3. to update the hardware alarm for the lastest alarm
 */
static __inline__ void
rtc_ualarm_adj(unsigned long old, unsigned long new)
{
    struct list_head  *head, *pos, *ppos;
    rtc_ualarm_t      rua;

    rtc_alarm_cli();

    head = &rtc_ualarm_list;
    ppos = head;
    list_for_each(pos, head) {
	rua = list_entry(pos, struct rtc_ualarm, u_list);

    	/* 1. to delete expired registered alarms if necessary */
	if (rua->u_time + rua->u_interval <= old) {
	    list_del(pos);
	    kfree(rua);

	    pos = ppos;
	}
	else {
   	   /*
	    * 2. to adjust registered alarms according to
	    *    the old and new system times
	    */
	    rua->u_interval -= old - rua->u_time;
	    rua->u_time      = new;

	    ppos = pos;
	}
    }

    /*
     * 3. to update the hardware alarm for the lastest alarm
     */
    head = &rtc_ualarm_list;
    pos  = head->next;
    rua  = list_entry(pos, struct rtc_ualarm, u_list);
    rtc_doset((unsigned int *)rALM_1, rua->u_time + rua->u_interval);

    rtc_alarm_sti();
} /* rtc_ualarm_adj() */

static __inline__ void
rtc_ualarm_next(void)
{
    unsigned long     nowtime = readw(rRTC);
    struct list_head  *head, *pos, *ppos;
    rtc_ualarm_t      rua;

    head = &rtc_ualarm_list;
    ppos = head;
    list_for_each(pos, head) {
	rua = list_entry(pos, struct rtc_ualarm, u_list);

    	/* 1. to delete expired registered alarms if necessary */
	if (rua->u_time + rua->u_interval > nowtime) break;

#if 0
	printk("(0x%x, %d) to be removed\n",
		(unsigned int)rua->u_time, (unsigned int)rua->u_interval);
#endif

	list_del(pos);
	kfree(rua);

	pos = ppos;
    }

    /*
     * 3. to update the hardware alarm for the lastest alarm
     */
    head = &rtc_ualarm_list;
    if (!list_empty(head)) {
	pos  = head->next;
	rua  = list_entry(pos, struct rtc_ualarm, u_list);
#if 0
	printk("(0x%x, %d) to be current\n", (unsigned int)rua->u_time,
		(unsigned int)rua->u_interval);
#endif
	rtc_doset((unsigned int *)rALM_1, rua->u_time + rua->u_interval);
    }
} /* rtc_ualarm_next() */

static void
rtc_ualarm_dump(void)
{
    struct list_head  *head, *pos;
    rtc_ualarm_t      rua;
    int		      i = 0;

    rtc_alarm_cli();

    head = &rtc_ualarm_list;
    list_for_each(pos, head) {
	rua = list_entry(pos, struct rtc_ualarm, u_list);
	printk("[%02d]: (u_time, u_interval) = (%lx, %lx)\n",
		i++, rua->u_time, rua->u_interval);
    }

    rtc_alarm_sti();
} /* rtc_ualarm_dump() */
#endif


/*
 *  common helper routines
 */

static __inline__ void
rtc_doset(unsigned int *addr, time_t tv_sec)
{
    int i;

#if 0
    writew(tv_sec, addr);
    for (i = 0; tv_sec != readw(addr) && i < 10000000; i++);
#else
    writel(tv_sec, addr);
    // ?? Wait for ok
#endif
} /* rtc_doset() */

static void
to_tm(unsigned long tim, struct rtc_time * tm)
{
#define FEBRUARY		2
#define STARTOFTIME		1970
#define SECDAY			86400L
#define SECYR			(SECDAY * 365)
#define leapyear(year)		((year) % 4 == 0)
#define days_in_year(a)		(leapyear(a) ? 366 : 365)
#define days_in_month(a)	(month_days[(a) - 1])

	static int month_days[12] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};

	long hms, day;
	int i;

	day = tim / SECDAY;
	hms = tim % SECDAY;

	/* Hours, minutes, seconds are easy */
	tm->tm_hour = hms / 3600;
	tm->tm_min = (hms % 3600) / 60;
	tm->tm_sec = (hms % 3600) % 60;

	/* Number of years in days */
	for (i = STARTOFTIME; day >= days_in_year(i); i++)
	day -= days_in_year(i);
	tm->tm_year = i;

	/* Number of months in days left */
	if (leapyear(tm->tm_year))
	days_in_month(FEBRUARY) = 29;
	for (i = 1; day >= days_in_month(i); i++)
	day -= days_in_month(i);
	days_in_month(FEBRUARY) = 28;
	tm->tm_mon = i;

	/* Days are what is left over (+1) from all that. */
	tm->tm_mday = day + 1;

	/*
	 * Determine the day of week
	 */
	tm->tm_wday = (day + 3) % 7;
} /* to_tm() */
