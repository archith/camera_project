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
static char *ps_name = "Prolific Power Switch Driver";
static char *ps_version = "1.0.0";
static char *ps_revdate = "2004-05-04";

#include <linux/config.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/kernel_stat.h>
#include <linux/sysctl.h>
#ifdef CONFIG_APM
#include <linux/apm_bios.h>
#include <linux/apm_ctl.h>
#endif /* CONFIG_APM */

#include <asm/irq.h>
#include <asm/hardware.h>


#define rPWRSW_INTCLR   (rTIMER_BASE + 0x0000000D)


/*
 *  * Debug macros
 *   */
#define PS_DEBUG 1

#ifdef PS_DEBUG
#define DPRINTK(fmt, args...)	printk("%s: " fmt, __FUNCTION__ , ## args)
#else
#define DPRINTK(fmt, args...)
#endif

/*
 * defaults
 */
#define PS_HOLD_SHUTDOWN	1
#define PS_HOLD_SUSPEND		1


/*
 * data structures
 */

typedef void (*ps_callback)(void);

struct ps_trigger {
	u_int	    duration;
	ps_callback callback;
};


/*
 * ps /proc Interface
 */

/*
 * forward references
 */
static ctl_table ps_root_table[];

static int ps_ctl(ctl_table *ctl, int write, struct file *filp,
		     void *buffer, size_t *lenp);


/*
 * Control name in various PS Control table
 */
enum {
	DEV_PS=1,
};

enum {
	PS_SHUTDOWN=1,
#ifdef CONFIG_APM
	PS_SUSPEND=2,
	PS_MAX=3,
#else
	PS_MAX=2,
#endif
};

/*
 * All stuff variables about control interface concerning PS
 */
static u_int ps_shutdown = PS_HOLD_SHUTDOWN;
static u_int ps_max_shutdown = 4;
static u_int ps_min_shutdown = 1;

#ifdef CONFIG_APM
static u_int ps_suspend = PS_HOLD_SUSPEND;
static u_int ps_max_suspend = 4;
static u_int ps_min_suspend = 1;
#endif /* CONFIG_APM */

static ctl_table ps_table[] = {
	{ PS_SHUTDOWN, "shutdown", &ps_shutdown,
          sizeof(ps_shutdown), 0644, NULL, &ps_ctl,
          &sysctl_intvec, NULL, &ps_min_shutdown, &ps_max_shutdown },

#ifdef CONFIG_APM
	{ PS_SUSPEND, "suspend", &ps_suspend,
          sizeof(ps_suspend), 0644, NULL, &ps_ctl,
          &sysctl_intvec, NULL, &ps_min_suspend, &ps_max_suspend },
#endif /* CONFIG_APM */

	{0}
};

static ctl_table ps_root[] = {
	{ DEV_PS, "ps", NULL, 0, 0555, ps_table },

	{0}
};

static ctl_table dev_root[] = {
	    { CTL_DEV, "dev", NULL, 0, 0555, ps_root, },

	    { 0, }
};

static struct ctl_table_header *ps_table_header;


/*
 * ps timing control
 */

/*
 * forward references
 */
static void ps_callback_shutdown(void);
#ifdef CONFIG_APM
static void ps_callback_suspend(void);
#endif /* CONFIG_APM */

/*
 * variables
 */
struct ps_trigger ps_trigger[PS_MAX] = {
	{ 0,		    (ps_callback)0	 },	/* NULL */
	{ PS_HOLD_SHUTDOWN * HZ, ps_callback_shutdown },/* PS_SHUTDOWN */
#ifdef CONFIG_APM
	{ PS_HOLD_SUSPEND * HZ,  ps_callback_suspend },	/* PS_SUSPEND */
#endif /* CONFIG_APM */
};

static u_int ps_max = 0;	/* the maximum duration to trigger action */
static u_int ps_hold = 0;	/* the current duration the power
			 	 * switch is held pressed to be returned ACK */
static u_int ps_interval = HZ >> 2;	/* the interval needed to poll
					 * the status of the power switch */

static struct timer_list ps_irq_timer;	/* to count how long the power switch
					 * is pressed */


/*
 * function prototypes
 */

/* forward */
static void ps_trigger_init(void);
static void ps_trigger_act(u_int hold);


/*
 * functions
 */

static inline u_int
ps_clear(void)
{
	volatile caddr_t psclr = (caddr_t)rPWRSW_INTCLR;

	*psclr = 0xFF;

	return (u_int)*psclr;
} /* ps_clear() */

static inline u_int
ps_status(void)
{
	volatile caddr_t psclr = (caddr_t)rPWRSW_INTCLR;

	return (u_int)*psclr;
} /* ps_status() */

static void
ps_timer_check(unsigned long data)
{
	ps_hold += ps_interval;

	if (ps_status() == 1) {
		if (ps_hold >= ps_max)
			ps_trigger_act(ps_hold);
		else
			mod_timer(&ps_irq_timer, jiffies + ps_interval);
	}
	else
		ps_trigger_act(ps_hold);
} /* ps_timer_check() */

static void
ps_interrupt(int irq, void *dev_instance, struct pt_regs *regs)
{
#if 0
	static int i = 0;
	printk(KERN_INFO "%s[%04d]: 0x%x\n", __FUNCTION__,
			i++, ps_clear());
#else
	ps_clear();
#endif

	ps_hold = 0;
	mod_timer(&ps_irq_timer, jiffies + ps_interval);
} /* ps_interrupt() */


/*
 * ps trigger
 */

static void
ps_trigger_init(void)
{
	int		  i;
	struct ps_trigger *pst;

	for (i = 1; i < PS_MAX; i++) {
		pst = &ps_trigger[i];
		if (ps_max < pst->duration) ps_max = pst->duration;
	}
} /* ps_trigger_init() */

static void
ps_trigger_act(u_int hold)
{
	int		  i;
	struct ps_trigger *pst, *tgt = (struct ps_trigger *)0;
	u_int		  min = 10 * HZ;

	for (i = 1; i < PS_MAX; i++) {
		pst = &ps_trigger[i];

		if (hold < pst->duration) continue;

		if (min > hold - pst->duration) {
			min = hold - pst->duration;
			tgt = pst;
		}
	}

	if (tgt != (struct ps_trigger *)0) tgt->callback();
} /* ps_trigger_act() */

static void
ps_trigger_update(int ctl_name, u_int duration)
{
	struct ps_trigger *pst;
	int		  i;

	if (!(ctl_name > 0 && ctl_name < PS_MAX)) return;

	pst = &ps_trigger[ctl_name];
	pst->duration = duration;

	/* to re-determine the value of ps_max */
	for (i = 1; i < PS_MAX; i++) {
		pst = &ps_trigger[i];
		if (ps_max < pst->duration) ps_max = pst->duration;
	}
} /* ps_trigger_update() */

static void
ps_callback_shutdown(void)
{
	/* ask busybox init to halt this system */
	kill_proc(1, SIGUSR1, 1);
} /* ps_callback_shutdown() */

#ifdef CONFIG_APM

static void
ps_callback_suspend(void)
{
	/* ask apmd to suspend this system */
	(void)apm_bios_event_enqueue(APM_SYS_SUSPEND);
} /* ps_callback_suspend() */

#endif /* CONFIG_APM */


/*
 * ps_init() friends
 */
static inline void
ps_show_version(void)
{
	printk(KERN_INFO "%s version %s (%s)\n",
			 ps_name, ps_version, ps_revdate);

} /* ps_show_version() */

static int __init
ps_init(void)
{
	int retval;

	/*
	 * show information about this ps driver
	 */
	ps_show_version();

	/*
	 * ps hardware-relevant initialization
	 */
	ps_clear();
	retval = request_irq(IRQ_PL_PW, ps_interrupt, SA_INTERRUPT, "ps", NULL);
    if (retval) return retval;

	/*
	 * /proc interface initialization
	 */
	ps_table_header = register_sysctl_table(dev_root, 0);

	/*
	 * ps trigger initialization
	 */
	ps_trigger_init();

	/*
	 * ps timing control initialization
	 */
	init_timer(&ps_irq_timer);

	ps_irq_timer.function = ps_timer_check;
	ps_irq_timer.expires = jiffies + ps_interval;
	add_timer(&ps_irq_timer);

	return 0;	/* success */
} /* ps_init() */

static void __exit
ps_cleanup(void)
{
	del_timer_sync(&ps_irq_timer);

	free_irq(IRQ_PL_PW, NULL);

	unregister_sysctl_table(ps_table_header);
} /* ps_cleanup() */

module_init(ps_init);
module_exit(ps_cleanup);


/*
 * /proc interface
 */
static int
ps_ctl(ctl_table *ctl, int write, struct file *filp, void *buffer,
	  size_t *lenp)
{
	int retval;

	retval = proc_dointvec_minmax(ctl, write, filp, buffer, lenp);
	if (retval != 0) return retval;

	switch (ctl->ctl_name) {
	case PS_SHUTDOWN:
		ps_trigger_update(PS_SHUTDOWN, ps_shutdown * HZ);
		break;
#ifdef CONFIG_APM
	case PS_SUSPEND:
		ps_trigger_update(PS_SUSPEND, ps_suspend * HZ);
		break;
#endif /* CONFIG_APM */
	}

	return 0;
} /* ps_ctl() */
