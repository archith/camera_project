/*
 *	Real Time Clock interface for Linux on CPE with FTRTC010
 *
 *	Based on sa1100-rtc.c by Nils Faerber
 *
 *	Based on rtc.c by Paul Gortmaker
 *	Date/time conversion routines taken from arch/arm/kernel/time.c
 *			by Linus Torvalds and Russel King
 *		and the GNU C Library
 *	( ... I love the GPL ... just take what you need! ;)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *	0.01	2004-11-17	I-Jui Sung <ijsung@faraday-tech.com>
 *	- initial release
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/ioport.h>
#include <asm/bitops.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/rtc.h>
#include <asm/arch/cpe/cpe.h>
#include <asm/arch-cpe/cpe/a320d.h>
#define	DRIVER_VERSION		"0.01"
#if 0
#define TIMER_FREQ		3686400

#define RTC_DEF_DIVIDER		32768 - 1
#define RTC_DEF_TRIM		0
#endif
/* FTRTC010 Register Offsets */
#define RtcSecond		0x0
#define RtcMinute		0x4
#define RtcHour			0x8
#define RtcDays			0xC
#define AlarmSecond		0x10
#define AlarmMinute		0x14
#define AlarmHour		0x18
#define RtcRecord		0x1C
#define RtcCR			0x20
#define RtcDivide		0x38
#define RtcRevision		0x3c

/* Those are the bits from a classic RTC we want to mimic */
#define RTC_IRQF		0x80	/* any of the following 3 is active */
#define RTC_PF			0x40
#define RTC_AF			0x20
#define RTC_UF			0x10

static unsigned long rtc_status;
static volatile unsigned long rtc_irq_data;
static unsigned long rtc_freq = 1;	/*FTRTC010 supports only 1Hz clock*/

static struct fasync_struct *rtc_async_queue;
static DECLARE_WAIT_QUEUE_HEAD(rtc_wait);

extern spinlock_t rtc_lock;

/* macro to get at IO space when running virtually */
//#define CPE_RTC_BASE 0x98600000
//#define CPE_RTC_VA_BASE IO_ADDRESS(CPE_RTC_BASE)

static const unsigned char days_in_mo[] =
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define is_leap(year) \
	((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

static unsigned getRTCuptime(void)
{
	return (inb(CPE_RTC_VA_BASE+RtcSecond)&0x3f)+60*(inb(CPE_RTC_VA_BASE+RtcMinute)&0x3f)+
		3600*(inb(CPE_RTC_VA_BASE+RtcHour)&0x1f)+86400*inw(CPE_RTC_VA_BASE+RtcDays);
}

/*
 * Converts seconds since 1970-01-01 00:00:00 to Gregorian date.
 */

static void decodetime (unsigned long t, struct rtc_time *tval)
{
        long days, month, year, rem;

        days = t / 86400;
        rem = t % 86400;
        tval->tm_hour = rem / 3600;
        rem %= 3600;
        tval->tm_min = rem / 60;
        tval->tm_sec = rem % 60;
        tval->tm_wday = (4 + days) % 7;

#define LEAPS_THRU_END_OF(y) ((y)/4 - (y)/100 + (y)/400)

	year = 1970 + days / 365;
	days -= ((year - 1970) * 365
			+ LEAPS_THRU_END_OF (year - 1)
			- LEAPS_THRU_END_OF (1970 - 1));
	if (days < 0) {
		year -= 1;
		days += 365 + is_leap(year);
	}
	tval->tm_year = year - 1900;
	tval->tm_yday = days + 1;

	month = 0;
	if (days >= 31) {
		days -= 31;
		month++;
		if (days >= (28 + is_leap(year))) {
			days -= (28 + is_leap(year));
			month++;
			while (days >= days_in_mo[month]) {
				days -= days_in_mo[month];
				month++;
			}
		}
	}
	tval->tm_mon = month;
	tval->tm_mday = days + 1;
	tval->tm_isdst=0;
}
static unsigned AIE_stat=0;
static unsigned saved_second=0; /* workaround for A320D */
/*ijsung:currently only support alarm*/
static void rtc_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
#if 0
	unsigned int rtsr = RTSR;

	/* clear interrupt sources */
	RTSR = 0;
	RTSR = (RTSR_AL|RTSR_HZ);

	/* clear alarm interrupt if it has occurred */
	if (rtsr & RTSR_AL)
		rtsr &= ~RTSR_ALE;
	RTSR = rtsr & (RTSR_ALE|RTSR_HZE);

	/* update irq data & counter */
	if (rtsr & RTSR_AL)
		rtc_irq_data |= (RTC_AF|RTC_IRQF);
	if (rtsr & RTSR_HZ)
		rtc_irq_data |= (RTC_UF|RTC_IRQF);
#else
	/* clear alarm interrupt source */
/*	outb((inb(CPE_RTC_VA_BASE+RtcCR) & (~0x20)), CPE_RTC_VA_BASE+RtcCR); */
	outb(63, CPE_RTC_VA_BASE+AlarmSecond); /* This is the only way to stop alarm in A320D */
/*	outb(0, CPE_RTC_VA_BASE+0x34);*/ /*clear intr state*/
	rtc_irq_data |= (RTC_AF|RTC_IRQF);
#endif
	rtc_irq_data += 0x100;

	/* wake up waiting process */
	wake_up_interruptible(&rtc_wait);
	kill_fasync (&rtc_async_queue, SIGIO, POLL_IN);
}
#if 0
static void timer1_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	/*
	 * If we match for the first time, the periodic interrupt flag won't
	 * be set.  If it is, then we did wrap around (very unlikely but
	 * still possible) and compute the amount of missed periods.
	 * The match reg is updated only when the data is actually retrieved
	 * to avoid unnecessary interrupts.
	 */
	OSSR = OSSR_M1;	/* clear match on timer1 */
	if (rtc_irq_data & RTC_PF) {
		rtc_irq_data += (rtc_freq * ((1<<30)/(TIMER_FREQ>>2))) << 8;
	} else {
		rtc_irq_data += (0x100|RTC_PF|RTC_IRQF);
	}

	wake_up_interruptible(&rtc_wait);
	kill_fasync (&rtc_async_queue, SIGIO, POLL_IN);
}
#endif

/*ijsung: arch-indep function*/
static int rtc_open(struct inode *inode, struct file *file)
{
	if (test_and_set_bit (1, &rtc_status))
		return -EBUSY;
	rtc_irq_data = 0;
	return 0;
}

static int rtc_release(struct inode *inode, struct file *file)
{
	spin_lock_irq (&rtc_lock);
	outb(63, CPE_RTC_VA_BASE+AlarmSecond); /* This is the only way to stop alarm in A320D */
	spin_unlock_irq (&rtc_lock);
	rtc_status = 0;
	return 0;
}

static int rtc_fasync (int fd, struct file *filp, int on)
{
	return fasync_helper (fd, filp, on, &rtc_async_queue);
}

static unsigned int rtc_poll(struct file *file, poll_table *wait)
{
	poll_wait (file, &rtc_wait, wait);
	return (rtc_irq_data) ? 0 : POLLIN | POLLRDNORM;
}

static loff_t rtc_llseek(struct file *file, loff_t offset, int origin)
{
	return -ESPIPE;
}

ssize_t rtc_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	DECLARE_WAITQUEUE(wait, current);
	unsigned long data;
	ssize_t retval;
	if (count < sizeof(unsigned long))
		return -EINVAL;

	add_wait_queue(&rtc_wait, &wait);
	set_current_state(TASK_INTERRUPTIBLE);
	for (;;) {
		spin_lock_irq (&rtc_lock);
		data = rtc_irq_data;
		if (data != 0) {
			rtc_irq_data = 0;
			break;
		}
		spin_unlock_irq (&rtc_lock);

		if (file->f_flags & O_NONBLOCK) {
			retval = -EAGAIN;
			goto out;
		}

		if (signal_pending(current)) {
			retval = -ERESTARTSYS;
			goto out;
		}

		schedule();
	}
#if 0
	if (data & RTC_PF) {
		/* interpolate missed periods and set match for the next one */
		unsigned long period = TIMER_FREQ/rtc_freq;
		unsigned long oscr = OSCR;
		unsigned long osmr1 = OSMR1;
		unsigned long missed = (oscr - osmr1)/period;
		data += missed << 8;
		OSSR = OSSR_M1;	/* clear match on timer 1 */
		OSMR1 = osmr1 + (missed + 1)*period;
		/* ensure we didn't miss another match in the mean time */
		while( (signed long)((osmr1 = OSMR1) - OSCR) <= 0 ) {
			data += 0x100;
			OSSR = OSSR_M1;	/* clear match on timer 1 */
			OSMR1 = osmr1 + period;
		}
	}
#endif
	spin_unlock_irq (&rtc_lock);

	data -= 0x100;	/* the first IRQ wasn't actually missed */

	retval = put_user(data, (unsigned long *)buf);
	if (!retval)
		retval = sizeof(unsigned long);

out:
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&rtc_wait, &wait);
	return retval;
}


static int rtc_ioctl(struct inode *inode, struct file *file,
		     unsigned int cmd, unsigned long arg)
{
	struct rtc_time tm, tm2;

	switch (cmd) {
	case RTC_AIE_OFF:
		spin_lock_irq(&rtc_lock);
		AIE_stat=0;
		outw(63, CPE_RTC_VA_BASE+AlarmSecond);
		rtc_irq_data = 0;
		spin_unlock_irq(&rtc_lock);
		return 0;
	case RTC_AIE_ON:
		spin_lock_irq(&rtc_lock);
		AIE_stat=1;
		outw(saved_second, CPE_RTC_VA_BASE+AlarmSecond);
		rtc_irq_data = 0;
		spin_unlock_irq(&rtc_lock);
		return 0;
#if 0
	case RTC_UIE_OFF:
		spin_lock_irq(&rtc_lock);
		RTSR &= ~RTSR_HZE;
		rtc_irq_data = 0;
		spin_unlock_irq(&rtc_lock);
		return 0;
	case RTC_UIE_ON:
		spin_lock_irq(&rtc_lock);
		RTSR |= RTSR_HZE;
		rtc_irq_data = 0;
		spin_unlock_irq(&rtc_lock);
		return 0;
	case RTC_PIE_OFF:
		spin_lock_irq(&rtc_lock);
		OIER &= ~OIER_E1;
		rtc_irq_data = 0;
		spin_unlock_irq(&rtc_lock);
		return 0;
	case RTC_PIE_ON:
		if ((rtc_freq > 64) && !capable(CAP_SYS_RESOURCE))
			return -EACCES;
		spin_lock_irq(&rtc_lock);
		OSMR1 = TIMER_FREQ/rtc_freq + OSCR;
		OIER |= OIER_E1;
		rtc_irq_data = 0;
		spin_unlock_irq(&rtc_lock);
		return 0;
#endif
	case RTC_ALM_READ:
		{	
			unsigned alarm_time=saved_second+
				60*(inb(CPE_RTC_VA_BASE+AlarmMinute)&0x3f)+
				3600*(inb(CPE_RTC_VA_BASE+AlarmHour)&0x3f);
			alarm_time+=inl(CPE_RTC_VA_BASE+RtcRecord);
			decodetime(alarm_time, &tm);
		}
		break;
	case RTC_ALM_SET:
		if (copy_from_user (&tm2, (struct rtc_time*)arg, sizeof (tm2)))
			return -EFAULT;
		{	
			unsigned user_alarm=mktime (tm2.tm_year+1900, tm2.tm_mon + 1, tm2.tm_mday, tm2.tm_hour, tm2.tm_min, tm2.tm_sec);
			user_alarm-=inl(CPE_RTC_VA_BASE+RtcRecord);
			decodetime(user_alarm, &tm2);
		}
		/*Need to calculate real alarm time=User supplied alarm time-record time*/		
		if ((unsigned)tm2.tm_hour < 24)
			outw(tm2.tm_hour, CPE_RTC_VA_BASE+AlarmHour);
		if ((unsigned)tm2.tm_min < 60)
			outw(tm2.tm_min, CPE_RTC_VA_BASE+AlarmMinute);
		saved_second=tm2.tm_sec;
		if(AIE_stat) {
			if ((unsigned)tm2.tm_sec < 60)
			        outw(tm2.tm_sec, CPE_RTC_VA_BASE+AlarmSecond);
		}
		return 0;
	case RTC_RD_TIME:
		{
			unsigned uptime=getRTCuptime();
			uptime+=inl(CPE_RTC_VA_BASE+RtcRecord);
			decodetime(uptime, &tm);	
		}
		break;
	case RTC_SET_TIME:
		{
			unsigned uptime=getRTCuptime(), usertime;
		
			if (!capable(CAP_SYS_TIME))
				return -EACCES;
			if (copy_from_user (&tm, (struct rtc_time*)arg, sizeof (tm)))
				return -EFAULT;
			tm.tm_year += 1900;
			if (tm.tm_year < 1970 || (unsigned)tm.tm_mon >= 12 ||
			    tm.tm_mday < 1 || tm.tm_mday > (days_in_mo[tm.tm_mon] +
					(tm.tm_mon == 1 && is_leap(tm.tm_year))) ||
			    (unsigned)tm.tm_hour >= 24 ||
			    (unsigned)tm.tm_min >= 60 ||
			    (unsigned)tm.tm_sec >= 60)
				return -EINVAL;
			usertime=mktime (tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
			outw((inw(CPE_RTC_VA_BASE+RtcCR) & (~0x1)), CPE_RTC_VA_BASE+RtcCR);
			outl(usertime-uptime, CPE_RTC_VA_BASE+RtcRecord);
			outw((inw(CPE_RTC_VA_BASE+RtcCR) | 0x1), CPE_RTC_VA_BASE+RtcCR);
		}
		return 0;

	case RTC_IRQP_READ:
		return put_user(rtc_freq, (unsigned long *)arg);
	case RTC_IRQP_SET:
#if 0
		if (arg < 1 || arg > TIMER_FREQ)
			        return -EINVAL;
		if ((arg > 64) && (!capable(CAP_SYS_RESOURCE)))
			        return -EACCES;
		rtc_freq = arg;
#else
		if (arg != 1) return -EINVAL;
#endif
		return 0;
	case RTC_EPOCH_READ:
		return put_user (1970, (unsigned long *)arg);
	default:
		return -EINVAL;
	}
	return copy_to_user ((void *)arg, &tm, sizeof (tm)) ? -EFAULT : 0;
}

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

static struct miscdevice ftrtc010rtc_miscdev = {
	RTC_MINOR,
	"rtc",
	&rtc_fops
};

static int rtc_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	unsigned alarm_time;
	char *p = page;
	int len;
	struct rtc_time tm;
	decodetime(getRTCuptime()+inl(CPE_RTC_VA_BASE+RtcRecord), &tm);
	p += sprintf(p, "rtc_time\t: %02d:%02d:%02d\n"
			"rtc_date\t: %04d-%02d-%02d\n"
			"rtc_epoch\t: %04d\n",
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 1970);
//	decodetime (RTAR, &tm);
	alarm_time=saved_second+60*(inb(CPE_RTC_VA_BASE+AlarmMinute)&0x3f)+3600*(inb(CPE_RTC_VA_BASE+AlarmHour)&0x3f);
	decodetime(alarm_time+inl(CPE_RTC_VA_BASE+RtcRecord), &tm);
	p += sprintf(p, "alrm_time\t: %02d:%02d:%02d\n"
			"alrm_date\t: N/A for CPE120\n",
			tm.tm_hour, tm.tm_min, tm.tm_sec);
//	p += sprintf(p, "trim/divider\t: 0x%08x\n", inl(CPE_RTC_VA_BASE+RtcDivide)>>1);
	p += sprintf(p, "alarm_IRQ\t: %s\n", AIE_stat ? "yes" : "no" );
//	p += sprintf(p, "update_IRQ\t: %s\n", (RTSR & RTSR_HZE) ? "yes" : "no");
//	p += sprintf(p, "periodic_IRQ\t: %s\n", (OIER & OIER_E1) ? "yes" : "no");
//	p += sprintf(p, "periodic_freq\t: %ld\n", rtc_freq);

	len = (p - page) - off;
	if (len < 0)
		len = 0;

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;

	return len;
}

static int __init rtc_init(void)
{
	int ret;
	misc_register (&ftrtc010rtc_miscdev);
	create_proc_read_entry ("driver/rtc", 0, 0, rtc_read_proc, NULL);
#if 0
	ret = request_irq (IRQ_RTC1Hz, rtc_interrupt, SA_INTERRUPT, "rtc 1Hz", NULL);
	if (ret) {
		printk (KERN_ERR "rtc: IRQ %d already in use.\n", IRQ_RTC1Hz);
		goto IRQ_RTC1Hz_failed;
	}
#endif
	ret = request_irq (17, rtc_interrupt, SA_INTERRUPT, "rtc Alrm", NULL);
	if (ret) {
		printk(KERN_ERR "rtc: IRQ %d already in use.\n", 17);
		goto IRQ_RTCAlrm_failed;
	}
#if 0
	ret = request_irq (IRQ_OST1, timer1_interrupt, SA_INTERRUPT, "rtc timer", NULL);
	if (ret) {
		printk(KERN_ERR "rtc: IRQ %d already in use.\n", IRQ_OST1);
		goto IRQ_OST1_failed;
	}
#endif
	printk (KERN_INFO "Faraday FTRTC010 Real Time Clock driver v" DRIVER_VERSION "\n");
	return 0;
#if 0
	/*
	 * According to the manual we should be able to let RTTR be zero
	 * and then a default diviser for a 32.768KHz clock is used.
	 * Apparently this doesn't work, at least for my SA1110 rev 5.
	 * If the clock divider is uninitialized then reset it to the
	 * default value to get the 1Hz clock.
	 */
	if (RTTR == 0) {
		RTTR = RTC_DEF_DIVIDER + (RTC_DEF_TRIM << 16);
		printk (KERN_WARNING "rtc: warning: initializing default clock divider/trim value\n");
		/*  The current RTC value probably doesn't make sense either */
		RCNR = 0;
	}

	return 0;

IRQ_OST1_failed:
	free_irq (IRQ_RTCAlrm, NULL);
#endif
IRQ_RTCAlrm_failed:
//	free_irq (IRQ_RTC1Hz, NULL);
IRQ_RTC1Hz_failed:
	remove_proc_entry ("driver/rtc", NULL);
	misc_deregister (&ftrtc010rtc_miscdev);
	return ret;
}

static void __exit rtc_exit(void)
{
#if 0
	free_irq (IRQ_OST1, NULL);
	free_irq (IRQ_RTCAlrm, NULL);
	free_irq (IRQ_RTC1Hz, NULL);
#endif
	remove_proc_entry ("driver/rtc", NULL);
	misc_deregister (&ftrtc010rtc_miscdev);
}

module_init(rtc_init);
module_exit(rtc_exit);

MODULE_AUTHOR("I-Jui Sung <ijsung@faraday-tech.com>");
MODULE_DESCRIPTION("CPE FTRTC010 Realtime Clock Driver (RTC)");
EXPORT_NO_SYMBOLS;
