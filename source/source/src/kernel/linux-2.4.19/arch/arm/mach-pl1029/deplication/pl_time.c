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
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/irq.h>

#include <asm/irq.h>
#include <asm/pl_reg.h>

#include <asm/arch/irqs.h>

extern rwlock_t xtime_lock;
extern volatile unsigned long wall_jiffies;

/* This is for machines which generate the exact clock. */
#define USECS_PER_JIFFY (1000000/HZ)

static unsigned long do_slow_gettimeoffset(void)
{
    return 0;
}

static unsigned long (*do_gettimeoffset)(void) = do_slow_gettimeoffset;

#ifndef CONFIG_PL1091_MMU
/*
 * This version of gettimeofday has near microsecond resolution.
 */
void do_gettimeofday(struct timeval *tv)
{
	unsigned long flags;

	read_lock_irqsave (&xtime_lock, flags);
	*tv = xtime;
	tv->tv_usec += do_gettimeoffset();

	/*
	 * xtime is atomically updated in timer_bh. jiffies - wall_jiffies
	 * is nonzero if the timer bottom half hasnt executed yet.
	 */
	if (jiffies - wall_jiffies)
		tv->tv_usec += USECS_PER_JIFFY;

	read_unlock_irqrestore (&xtime_lock, flags);

	if (tv->tv_usec >= 1000000) {
		tv->tv_usec -= 1000000;
		tv->tv_sec++;
	}
}

void do_settimeofday(struct timeval *tv)
{
	write_lock_irq (&xtime_lock);
	/* This is revolting. We need to set the xtime.tv_usec
	 * correctly. However, the value in this location is
	 * is value at the last tick.
	 * Discover what correction gettimeofday
	 * would have done, and then undo it!
	 */
	tv->tv_usec -= do_gettimeoffset();

	if (tv->tv_usec < 0) {
		tv->tv_usec += 1000000;
		tv->tv_sec--;
	}

	xtime = *tv;
	time_adjust = 0;		/* stop active adjtime() */
	time_status |= STA_UNSYNC;
	time_maxerror = NTP_PHASE_LIMIT;
	time_esterror = NTP_PHASE_LIMIT;
	write_unlock_irq (&xtime_lock);
}
#else
void do_gettimeofday(struct timeval *tv)
{
	unsigned long flags;
	unsigned long usec, sec;

	read_lock_irqsave(&xtime_lock, flags);
	usec = do_gettimeoffset();
	{
		unsigned long lost = jiffies - wall_jiffies;

		if (lost)
			usec += lost * USECS_PER_JIFFY;
	}
	sec = xtime.tv_sec;
	usec += xtime.tv_usec;
	read_unlock_irqrestore(&xtime_lock, flags);

	/* usec may have gone up a lot: be safe */
	while (usec >= 1000000) {
		usec -= 1000000;
		sec++;
	}

	tv->tv_sec = sec;
	tv->tv_usec = usec;
}

void do_settimeofday(struct timeval *tv)
{
	write_lock_irq(&xtime_lock);
	/* This is revolting. We need to set the xtime.tv_usec
	 * correctly. However, the value in this location is
	 * is value at the last tick.
	 * Discover what correction gettimeofday
	 * would have done, and then undo it!
	 */
	tv->tv_usec -= do_gettimeoffset();
	tv->tv_usec -= (jiffies - wall_jiffies) * USECS_PER_JIFFY;

	while (tv->tv_usec < 0) {
		tv->tv_usec += 1000000;
		tv->tv_sec--;
	}

	xtime = *tv;
	time_adjust = 0;		/* stop active adjtime() */
	time_status |= STA_UNSYNC;
	time_maxerror = NTP_PHASE_LIMIT;
	time_esterror = NTP_PHASE_LIMIT;
	write_unlock_irq(&xtime_lock);
}
#endif /* CONFIG_PL1091_MMU */


/* last time the RTC clock got updated */
static long last_rtc_update;

/*
 * timer_interrup needs to keep up the real-time clock,
 * as well as call the "do_timer()" routine every clocktick
 */

static void inline
pl_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    /* clear system tick interrupt signal */
    byteAddr(rTICKER_CLEAR) = 0;

#if defined(CONFIG_ARCH_PL1071) || defined(CONFIG_ARCH_PL1091)

#else
    if (!user_mode(regs)) {
        if (prof_buffer && current->pid) {
            extern int _stext;
            unsigned long pc = regs->cp0_epc;

            pc -= (unsigned long) & _stext;
            pc >>= prof_shift;
            /*
             * Dont ignore out-of-bounds pc values silently,
             * put them into the last histogram slot, so if
             * present, they will show up as a sharp peak.
             */
            if (pc > prof_len - 1)
                pc = prof_len -1;
            atomic_inc((atomic_t *) &prof_buffer[pc]);
        }
    }
#endif  /* !CONFIG_ARCH_PL1071 && !CONFIG_ARCH_PL1091 */
    do_timer(regs);
    /*
     * If we have an externally synchronized Linux clock, then update
     * CMOS clock accordingly every ~11 minutes. Set_rtc_mmss() has to be
     * called as close as possible to next sec before the new second starts.
     */
    read_lock(&xtime_lock);
    if ((time_status & STA_UNSYNC) == 0 &&
        xtime.tv_sec > last_rtc_update + 600 &&
        xtime.tv_usec > 1000000 - (tick) &&
        xtime.tv_usec < 1000000 - (tick >> 1))
        if (xtime.tv_sec != wordAddr(rRTC)) {
            wordAddr(rRTC) = xtime.tv_sec;
            last_rtc_update = xtime.tv_sec;
        }
    /* As we return to user mode fire off the other CPU schedulers.. this is
       basically because we don't yet share IRQ's around.  This message is
       rigged to be safe on the 386 - basically it's a hack, so don't look
       closely for now..*/
    /*smp_message_pass(MSG_ALL_BUT_SELF, MSG_RESCHEDULE, 0L, 0); */
    read_unlock(&xtime_lock);

}



struct irqaction irq0  = { pl_timer_interrupt, SA_INTERRUPT, 0,
                                  "timer", NULL, NULL};

extern  void (*board_time_init)(struct irqaction *irq);



void pl_time_init(struct irqaction *irq)
{
    irq->handler = pl_timer_interrupt;

    /* setup isr */
#if defined(CONFIG_ARCH_PL1071) || defined(CONFIG_ARCH_PL1091)
    if (setup_arm_irq(IRQ_PL_TIMER, &irq0))
        panic("time_init: unable request irq for system timer");
#else     
    if (setup_irq(IRQ_PL_TIMER, &irq0))
        panic("time_init: unable request irq for system timer");
#endif         

    /* setup PIT */
#if defined(CONFIG_ARCH_PL1071) || defined(CONFIG_ARCH_PL1091)
    byteAddr(rTICKER_PERIOD) = LOG_2_HZ - 4 ;
    byteAddr(rTICKER_START) = 1; /* Enable system ticks */
#else
    byteAddr(rTICKER_PERIOD) = LOG_2_HZ - 4;
    byteAddr(rTICKER_CLEAR) = 0;
    byteAddr(rTICKER_START) = 1;
#endif

}



void time_init(void)
{
    unsigned int sec;
    int i;

    /* wait the RTC changing */
    sec = wordAddr(rRTC);
    for (i = 0; sec == wordAddr(rRTC) && i < 10000000; i++);
    sec = wordAddr(rRTC);

	write_lock_irq (&xtime_lock);
	xtime.tv_sec = sec;
	xtime.tv_usec = 0;
	write_unlock_irq (&xtime_lock);

	board_time_init(&irq0);
}


