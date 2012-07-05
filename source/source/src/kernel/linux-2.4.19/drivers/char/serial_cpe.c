// --------------------------------------------------------------------
//		2002-11-26: lmc83: modified from serial.c
// --------------------------------------------------------------------
extern void cpe_int_set_irq(unsigned int,int,int);
static char *serial_version = "5.05c";
static char *serial_revdate = "2001-07-08";

#include <linux/config.h>
#include <linux/version.h>
#include <asm/arch/cpe/cpe.h>
#include <asm/arch/cpe_int.h>
#include "serial_cpe.h"

#undef SERIAL_PARANOIA_CHECK
#define CONFIG_SERIAL_NOPAUSE_IO
#define SERIAL_DO_RESTART

#ifdef CONFIG_SERIAL_ACPI
#define ENABLE_SERIAL_ACPI
#endif

/* Set of debugging defines */

#undef SERIAL_DEBUG_INTR
//#define SERIAL_DEBUG_OPEN
#undef SERIAL_DEBUG_OPEN
#define SERIAL_DEBUG_FLOW
//#undef SERIAL_DEBUG_FLOW
#undef SERIAL_DEBUG_RS_WAIT_UNTIL_SENT
#undef SERIAL_DEBUG_PCI
#undef SERIAL_DEBUG_AUTOCONF


#ifdef MODULE
#undef CONFIG_SERIAL_CPE_CONSOLE
#endif


#define RS_STROBE_TIME 		(10*HZ)
//#define RS_STROBE_TIME 		HZ
#define RS_ISR_PASS_LIMIT 	256

  
/*
 * End of serial driver configuration section.
 */

#include <linux/module.h>

#include <linux/types.h>
#ifdef LOCAL_HEADERS
#include "serial_local.h"
#else
#include <linux/serial.h>
#include <linux/serialP.h>
#include <linux/serial_reg.h>
#include <asm/serial.h>
#define LOCAL_VERSTRING ""
#endif

#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/slab.h>
#if (LINUX_VERSION_CODE >= 131343)
#include <linux/init.h>
#endif
#if (LINUX_VERSION_CODE >= 131336)
#include <asm/uaccess.h>
#endif
#include <linux/delay.h>
#ifdef CONFIG_SERIAL_CPE_CONSOLE
#include <linux/console.h>
#endif
#ifdef CONFIG_MAGIC_SYSRQ
#include <linux/sysrq.h>
#endif

/*
 * All of the compatibilty code so we can compile serial.c against
 * older kernels is hidden in serial_compat.h
 */
#if defined(LOCAL_HEADERS) || (LINUX_VERSION_CODE < 0x020317) /* 2.3.23 */
#include "serial_compat.h"
#endif

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/bitops.h>

#ifdef CONFIG_MAC_SERIAL
#define SERIAL_DEV_OFFSET	2
#else
#define SERIAL_DEV_OFFSET	0
#endif

#ifdef SERIAL_INLINE
#define _INLINE_ inline
#else
#define _INLINE_
#endif

static char *serial_name = "Serial driver";

static DECLARE_TASK_QUEUE(tq_serial);

static struct tty_driver serial_driver, callout_driver;
static int serial_refcount;

static struct timer_list serial_timer;

//ivan
static int show_once=1;

/* serial subtype definitions */
#ifndef SERIAL_TYPE_NORMAL
#define SERIAL_TYPE_NORMAL	1
#define SERIAL_TYPE_CALLOUT	2
#endif

/* number of characters left in xmit buffer before we ask for more */
#define WAKEUP_CHARS 256

/*
 * IRQ_timeout		- How long the timeout should be for each IRQ
 * 				should be after the IRQ has been active.
 */

static struct async_struct *IRQ_ports[NR_IRQS];
static int IRQ_timeout[NR_IRQS];
#ifdef CONFIG_SERIAL_CPE_CONSOLE
static struct console sercons;
static int lsr_break_flag;
#endif
#if defined(CONFIG_SERIAL_CPE_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
static unsigned long break_pressed; /* break, really ... */
#endif

static unsigned detect_uart_irq (struct serial_state * state);
static void autoconfig(struct serial_state * state);
static void change_speed(struct async_struct *info, struct termios *old);
static void rs_wait_until_sent(struct tty_struct *tty, int timeout);

/*
 * Here we define the default xmit fifo size used for each type of
 * UART
 */
// name, fifosize, flags
static struct serial_uart_config uart_config[] = {
	{ "unknown", 1, 0 }, 
	{ "8250", 1, 0 }, 
	{ "16450", 1, 0 }, 
	{ "16550", 1, 0 }, 
	{ "16550A", 16, UART_CLEAR_FIFO | UART_USE_FIFO }, 
	{ "cirrus", 1, 0 }, 	/* usurped by cyclades.c */
	{ "ST16650", 1, UART_CLEAR_FIFO | UART_STARTECH }, 
	{ "ST16650V2", 32, UART_CLEAR_FIFO | UART_USE_FIFO |
		  UART_STARTECH }, 
	{ "TI16750", 64, UART_CLEAR_FIFO | UART_USE_FIFO},
	{ "16C950/954", 128, UART_CLEAR_FIFO | UART_USE_FIFO},
	{ "ST16654", 64, UART_CLEAR_FIFO | UART_USE_FIFO |
		  UART_STARTECH }, 
	{ "XR16850", 128, UART_CLEAR_FIFO | UART_USE_FIFO |
		  UART_STARTECH },
	{ "RSA", 2048, UART_CLEAR_FIFO | UART_USE_FIFO }, 
	//{ "FaradayUART", 1, UART_CLEAR_FIFO},
	{ "FaradayUART", 16, UART_CLEAR_FIFO|UART_USE_FIFO },
	{ 0, 0}
};



static struct serial_state rs_table[RS_TABLE_SIZE] = {
	SERIAL_PORT_DFNS
};

#define NR_PORTS	(sizeof(rs_table)/sizeof(struct serial_state))


#ifndef PREPARE_FUNC
#define PREPARE_FUNC(dev)  (dev->prepare)
#define ACTIVATE_FUNC(dev)  (dev->activate)
#define DEACTIVATE_FUNC(dev)  (dev->deactivate)
#endif

#define HIGH_BITS_OFFSET ((sizeof(long)-sizeof(int))*8)

static struct tty_struct *serial_table[NR_PORTS];
static struct termios *serial_termios[NR_PORTS];
static struct termios *serial_termios_locked[NR_PORTS];


#if defined(MODULE) && defined(SERIAL_DEBUG_MCOUNT)
#define DBG_CNT(s) printk("(%s): [%x] refc=%d, serc=%d, ttyc=%d -> %s\n", \
 kdevname(tty->device), (info->flags), serial_refcount,info->count,tty->count,s)
#else
#define DBG_CNT(s)
#endif

/*
 * tmp_buf is used as a temporary buffer by serial_write.  We need to
 * lock it in case the copy_from_user blocks while swapping in a page,
 * and some other program tries to do a serial write at the same time.
 * Since the lock will only come under contention when the system is
 * swapping and available memory is low, it makes sense to share one
 * buffer across all the serial ports, since it significantly saves
 * memory if large numbers of serial ports are open.
 */
static unsigned char *tmp_buf;
#ifdef DECLARE_MUTEX
static DECLARE_MUTEX(tmp_buf_sem);
#else
static struct semaphore tmp_buf_sem = MUTEX;
#endif


static inline int serial_paranoia_check(struct async_struct *info,
					kdev_t device, const char *routine)
{
#ifdef SERIAL_PARANOIA_CHECK
	static const char *badmagic =
		"Warning: bad magic number for serial struct (%s) in %s\n";
	static const char *badinfo =
		"Warning: null async_struct for (%s) in %s\n";

	if (!info) {
		printk(badinfo, kdevname(device), routine);
		return 1;
	}
	if (info->magic != SERIAL_MAGIC) {
		printk(badmagic, kdevname(device), routine);
		return 1;
	}
#endif
	return 0;
}

static _INLINE_ unsigned int serial_in(struct async_struct *info, int offset)
{
	return inb(info->port + offset);
}

static _INLINE_ void serial_out(struct async_struct *info, int offset,
				int value)
{
	outb(value, info->port+offset);
}

/*
 * We used to support using pause I/O for certain machines.  We
 * haven't supported this for a while, but just in case it's badly
 * needed for certain old 386 machines, I've left these #define's
 * in....
 */
#define serial_inp(info, offset)		serial_in(info, offset)
#define serial_outp(info, offset, value)	serial_out(info, offset, value)


/*
 * For the 16C950
 */
void serial_icr_write(struct async_struct *info, int offset, int  value)
{
	serial_out(info, UART_SCR, offset);
	serial_out(info, UART_ICR, value);
}

unsigned int serial_icr_read(struct async_struct *info, int offset)
{
	int	value;

	serial_icr_write(info, UART_ACR, info->ACR | UART_ACR_ICRRD);
	serial_out(info, UART_SCR, offset);
	value = serial_in(info, UART_ICR);
	serial_icr_write(info, UART_ACR, info->ACR);
	return value;
}

/*
 * ------------------------------------------------------------
 * rs_stop() and rs_start()
 *
 * This routines are called before setting or resetting tty->stopped.
 * They enable or disable transmitter interrupts, as necessary.
 * ------------------------------------------------------------
 */
static void rs_stop(struct tty_struct *tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rs_stop"))
		return;
	
	save_flags(flags); cli();
	if (info->IER & SERIAL_IER_TE) {
		info->IER &= ~SERIAL_IER_TE;
		serial_out(info, SERIAL_IER, info->IER);
	}
	if (info->state->type == PORT_16C950) {
		info->ACR |= UART_ACR_TXDIS;
		serial_icr_write(info, UART_ACR, info->ACR);
	}
	restore_flags(flags);
}

static void rs_start(struct tty_struct *tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	unsigned long flags;
	
	if (serial_paranoia_check(info, tty->device, "rs_start"))
		return;
	
	save_flags(flags); cli();
	if (info->xmit.head != info->xmit.tail
	    && info->xmit.buf
	    && !(info->IER & SERIAL_IER_TE)) {
		info->IER |= SERIAL_IER_TE;
		serial_out(info, SERIAL_IER, info->IER);
	}
	if (info->state->type == PORT_16C950) {
		info->ACR &= ~UART_ACR_TXDIS;
		serial_icr_write(info, UART_ACR, info->ACR);
	}
	restore_flags(flags);
}

/*
 * ----------------------------------------------------------------------
 *
 * Here starts the interrupt handling routines.  All of the following
 * subroutines are declared as inline and are folded into
 * rs_interrupt().  They were separated out for readability's sake.
 *
 * Note: rs_interrupt() is a "fast" interrupt, which means that it
 * runs with interrupts turned off.  People who may want to modify
 * rs_interrupt() should try to keep the interrupt handler as fast as
 * possible.  After you are done making modifications, it is not a bad
 * idea to do:
 * 
 * gcc -S -DKERNEL -Wall -Wstrict-prototypes -O6 -fomit-frame-pointer serial.c
 *
 * and look at the resulting assemble code in serial.s.
 *
 * 				- Ted Ts'o (tytso@mit.edu), 7-Mar-93
 * -----------------------------------------------------------------------
 */

/*
 * This routine is used by the interrupt handler to schedule
 * processing in the software interrupt portion of the driver.
 */
static _INLINE_ void rs_sched_event(struct async_struct *info,
				  int event)
{
	info->event |= 1 << event;
	queue_task(&info->tqueue, &tq_serial);
	mark_bh(SERIAL_BH);
}

static _INLINE_ void receive_chars(struct async_struct *info,
				 int *status, struct pt_regs * regs)
{
	struct tty_struct *tty = info->tty;
	unsigned char ch;
	struct	async_icount *icount;
	int	max_count = 256;


	icount = &info->state->icount;
	do {
		if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
			tty->flip.tqueue.routine((void *) tty);
			if (tty->flip.count >= TTY_FLIPBUF_SIZE)
				return;		// if TTY_DONT_FLIP is set
		}
		ch = serial_inp(info, SERIAL_RBR);
		*tty->flip.char_buf_ptr = ch;
		icount->rx++;
		
#ifdef SERIAL_DEBUG_INTR
		printk("DR%02x:%02x...", ch, *status);
#endif
		*tty->flip.flag_buf_ptr = 0;
		if (*status & (SERIAL_LSR_BI | SERIAL_LSR_PE |
			       SERIAL_LSR_FE | SERIAL_LSR_OE)) {
			/*
			 * For statistics only
			 */
			if (*status & SERIAL_LSR_BI) {
				*status &= ~(SERIAL_LSR_FE | SERIAL_LSR_PE);
				icount->brk++;
				/*
				 * We do the SysRQ and SAK checking
				 * here because otherwise the break
				 * may get masked by ignore_status_mask
				 * or read_status_mask.
				 */
#if defined(CONFIG_SERIAL_CPE_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
				if (info->line == sercons.index) {
					if (!break_pressed) {
						break_pressed = jiffies;
						goto ignore_char;
					}
					break_pressed = 0;
				}
#endif
				if (info->flags & ASYNC_SAK)
					do_SAK(tty);
			} else if (*status & SERIAL_LSR_PE)
				icount->parity++;
			else if (*status & SERIAL_LSR_FE)
				icount->frame++;
			if (*status & SERIAL_LSR_OE)
				icount->overrun++;

			/*
			 * Mask off conditions which should be ignored.
			 */
			*status &= info->read_status_mask;

#ifdef CONFIG_SERIAL_CPE_CONSOLE
			if (info->line == sercons.index) {
				/* Recover the break flag from console xmit */
				*status |= lsr_break_flag;
				lsr_break_flag = 0;
			}
#endif
			if (*status & (SERIAL_LSR_BI)) {
#ifdef SERIAL_DEBUG_INTR
				printk("handling break....");
#endif
				*tty->flip.flag_buf_ptr = TTY_BREAK;
			} else if (*status & SERIAL_LSR_PE)
				*tty->flip.flag_buf_ptr = TTY_PARITY;
			else if (*status & SERIAL_LSR_FE)
				*tty->flip.flag_buf_ptr = TTY_FRAME;
		}
#if defined(CONFIG_SERIAL_CPE_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
		if (break_pressed && info->line == sercons.index) {
			if (ch != 0 &&
			    time_before(jiffies, break_pressed + HZ*5)) {
				handle_sysrq(ch, regs, NULL, NULL);
				break_pressed = 0;
				goto ignore_char;
			}
			break_pressed = 0;
		}
#endif
		if ((*status & info->ignore_status_mask) == 0) {
			tty->flip.flag_buf_ptr++;
			tty->flip.char_buf_ptr++;
			tty->flip.count++;
		}
		if ((*status & SERIAL_LSR_OE) &&
		    (tty->flip.count < TTY_FLIPBUF_SIZE)) {
			/*
			 * Overrun is special, since it's reported
			 * immediately, and doesn't affect the current
			 * character
			 */
			*tty->flip.flag_buf_ptr = TTY_OVERRUN;
			tty->flip.count++;
			tty->flip.flag_buf_ptr++;
			tty->flip.char_buf_ptr++;
		}
#if defined(CONFIG_SERIAL_CPE_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
	ignore_char:
#endif
		*status = serial_inp(info, SERIAL_LSR);
	} while ((*status & SERIAL_LSR_DR) && (max_count-- > 0));
#if (LINUX_VERSION_CODE > 131394) /* 2.1.66 */
	tty_flip_buffer_push(tty);
#else
	queue_task_irq_off(&tty->flip.tqueue, &tq_timer);
#endif	
}

static _INLINE_ void transmit_chars(struct async_struct *info, int *intr_done)
{
	int count;

	if (info->x_char) 
	{
		serial_outp(info, SERIAL_THR, info->x_char);
		info->state->icount.tx++;
		info->x_char = 0;
		if (intr_done)
			*intr_done = 0;
		return;
	}
	if (info->xmit.head == info->xmit.tail
	    || info->tty->stopped
	    || info->tty->hw_stopped) 
	{
		info->IER &= ~SERIAL_IER_TE;
		serial_out(info, SERIAL_IER, info->IER);
		return;
	}

	
#ifdef not_complete_yet	
	for (; info->xmit.tail != info->xmit.head; )
	{
		wait_for_xmitr(info);
		serial_out(info, SERIAL_THR, info->xmit.buf[info->xmit.tail]);
		info->xmit.tail = (info->xmit.tail + 1) & (SERIAL_XMIT_SIZE-1);
		info->state->icount.tx++;
	}		
#endif /* end_of_not */
	count = info->xmit_fifo_size;
	do {
		serial_out(info, SERIAL_THR, info->xmit.buf[info->xmit.tail]);
		info->xmit.tail = (info->xmit.tail + 1) & (SERIAL_XMIT_SIZE-1);
		info->state->icount.tx++;
		if (info->xmit.head == info->xmit.tail)
			break;
	} while (--count > 0);

	if (CIRC_CNT(info->xmit.head,
		     info->xmit.tail,
		     SERIAL_XMIT_SIZE) < WAKEUP_CHARS)
	{
		rs_sched_event(info, RS_EVENT_WRITE_WAKEUP);
	}

#ifdef SERIAL_DEBUG_INTR
	printk("THRE...");
#endif
	if (intr_done)
		*intr_done = 0;

	if (info->xmit.head == info->xmit.tail) {
		info->IER &= ~SERIAL_IER_TE;
		serial_out(info, SERIAL_IER, info->IER);
	}
}

static _INLINE_ void check_modem_status(struct async_struct *info)
{
	int	status;
	struct	async_icount *icount;
	
	status = serial_in(info, SERIAL_MSR);

	if (status & SERIAL_MSR_ANY_DELTA) {
		icount = &info->state->icount;
		/* update input line counters */
		if (status & SERIAL_MSR_TERI)
			icount->rng++;
		if (status & SERIAL_MSR_DELTADSR)
			icount->dsr++;
		if (status & SERIAL_MSR_DELTACD) {
			icount->dcd++;
#ifdef CONFIG_HARD_PPS
			if ((info->flags & ASYNC_HARDPPS_CD) &&
			    (status & SERIAL_MSR_DCD))
				hardpps();
#endif
		}
		if (status & SERIAL_MSR_DELTACTS)
			icount->cts++;
		wake_up_interruptible(&info->delta_msr_wait);
	}

	if ((info->flags & ASYNC_CHECK_CD) && (status & SERIAL_MSR_DELTACD)) {
#if (defined(SERIAL_DEBUG_OPEN) || defined(SERIAL_DEBUG_INTR))
		printk("ttys%d CD now %s...", info->line,
		       (status & SERIAL_MSR_DCD) ? "on" : "off");
#endif		
		if (status & SERIAL_MSR_DCD)
			wake_up_interruptible(&info->open_wait);
		else if (!((info->flags & ASYNC_CALLOUT_ACTIVE) &&
			   (info->flags & ASYNC_CALLOUT_NOHUP))) {
#ifdef SERIAL_DEBUG_OPEN
			printk("doing serial hangup...");
#endif
			if (info->tty)
				tty_hangup(info->tty);
		}
	}
	if (info->flags & ASYNC_CTS_FLOW) {
		if (info->tty->hw_stopped) {
			if (status & SERIAL_MSR_CTS) {
#if (defined(SERIAL_DEBUG_INTR) || defined(SERIAL_DEBUG_FLOW))
				printk("CTS tx start...");
#endif
				info->tty->hw_stopped = 0;
				info->IER |= SERIAL_IER_TE;
				serial_out(info, SERIAL_IER, info->IER);
				rs_sched_event(info, RS_EVENT_WRITE_WAKEUP);
				return;
			}
		} else {
			if (!(status & SERIAL_MSR_CTS)) {
#if (defined(SERIAL_DEBUG_INTR) || defined(SERIAL_DEBUG_FLOW))
				printk("CTS tx stop...");
#endif
				info->tty->hw_stopped = 1;
				info->IER &= ~SERIAL_IER_TE;
				serial_out(info, SERIAL_IER, info->IER);
			}
		}
	}
}

#ifdef not_complete_yet
#ifdef CONFIG_SERIAL_SHARE_IRQ
/*
 * This is the serial driver's generic interrupt routine
 */
static void rs_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	int status;
	struct async_struct * info;
	int pass_counter = 0;
	struct async_struct *end_mark = 0;

#ifdef SERIAL_DEBUG_INTR
	printk("rs_interrupt(%d)...", irq);
#endif

	info = IRQ_ports[irq];
	if (!info)
		return;

	do {
		if (!info->tty ||
		    (serial_in(info, SERIAL_IIR) & SERIAL_IIR_NONE)) {
			if (!end_mark)
				end_mark = info;
			goto next;
		}
#ifdef SERIAL_DEBUG_INTR
		printk("IIR = %x...", serial_in(info, SERIAL_IIR));
#endif
		end_mark = 0;

		info->last_active = jiffies;

		status = serial_inp(info, SERIAL_LSR);
#ifdef SERIAL_DEBUG_INTR
		printk("status = %x...", status);
#endif
		if (status & SERIAL_LSR_DR)
			receive_chars(info, &status, regs);
		check_modem_status(info);
		if (status & SERIAL_LSR_THRE)
			transmit_chars(info, 0);

	next:
		info = info->next_port;
		if (!info) {
			info = IRQ_ports[irq];
			if (pass_counter++ > RS_ISR_PASS_LIMIT) {
#if 0
				printk("rs loop break\n");
#endif
				break; 	/* Prevent infinite loops */
			}
			continue;
		}
	} while (end_mark != info);

#ifdef SERIAL_DEBUG_INTR
	printk("end.\n");
#endif
}
#endif /* #ifdef CONFIG_SERIAL_SHARE_IRQ */
#endif /* end_of_not */

/*
 * This is the serial driver's interrupt routine for a single port
 */
static void rs_interrupt_single(int irq, void *dev_id, struct pt_regs * regs)
{
	int status;
	int pass_counter = 0;
	struct async_struct * info;
	
#ifdef SERIAL_DEBUG_INTR
	printk("rs_interrupt_single(%d)...", irq);
#endif

	info = IRQ_ports[irq];
	if (!info || !info->tty)
		return;


	do {
		status = serial_inp(info, SERIAL_LSR);
#ifdef SERIAL_DEBUG_INTR
		printk("status = %x...", status);
#endif
		if (status & SERIAL_LSR_DR)
			receive_chars(info, &status, regs);
		check_modem_status(info);
		if (status & SERIAL_LSR_THRE)
			transmit_chars(info, 0);
		if (pass_counter++ > RS_ISR_PASS_LIMIT) {
#if 0
			printk("rs_single loop break.\n");
#endif
			break;
		}
#ifdef SERIAL_DEBUG_INTR
		printk("IIR = %x...", serial_in(info, SERIAL_IIR));
#endif
	} while (!(serial_in(info, SERIAL_IIR) & SERIAL_IIR_NONE));
	info->last_active = jiffies;

#ifdef SERIAL_DEBUG_INTR
	printk("end.\n");
#endif
}



/*
 * -------------------------------------------------------------------
 * Here ends the serial interrupt routines.
 * -------------------------------------------------------------------
 */

/*
 * This routine is used to handle the "bottom half" processing for the
 * serial driver, known also the "software interrupt" processing.
 * This processing is done at the kernel interrupt level, after the
 * rs_interrupt() has returned, BUT WITH INTERRUPTS TURNED ON.  This
 * is where time-consuming activities which can not be done in the
 * interrupt driver proper are done; the interrupt driver schedules
 * them using rs_sched_event(), and they get done here.
 */
static void do_serial_bh(void)
{
//printk("do_serial_bh\r\n");
	run_task_queue(&tq_serial);
}

static void do_softint(void *private_)
{
	struct async_struct	*info = (struct async_struct *) private_;
	struct tty_struct	*tty;
//printk("do_softint\r\n");
	tty = info->tty;
	if (!tty)
		return;

	if (test_and_clear_bit(RS_EVENT_WRITE_WAKEUP, &info->event)) 
	{
		if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
		    tty->ldisc.write_wakeup)
			(tty->ldisc.write_wakeup)(tty);
		wake_up_interruptible(&tty->write_wait);
#ifdef SERIAL_HAVE_POLL_WAIT
		wake_up_interruptible(&tty->poll_wait);
#endif
	}
}


/*
 * This subroutine is called when the RS_TIMER goes off.  It is used
 * by the serial driver to handle ports that do not have an interrupt
 * (irq=0).  This doesn't work very well for 16450's, but gives barely
 * passable results for a 16550A.  (Although at the expense of much
 * CPU overhead).
 */
static void rs_timer(unsigned long dummy)
{
	static unsigned long last_strobe;
	struct async_struct *info;
	unsigned int	i;
	unsigned long flags;
//printk("rs_timer\r\n");
	if ((jiffies - last_strobe) >= RS_STROBE_TIME) 
	{
		for (i=0; i < NR_IRQS; i++) 
		{
			info = IRQ_ports[i];
			if (!info)
				continue;
			save_flags(flags); cli();
			rs_interrupt_single(i, NULL, NULL);
			restore_flags(flags);
		}
	}
	last_strobe = jiffies;
	mod_timer(&serial_timer, jiffies + RS_STROBE_TIME);

	if (IRQ_ports[0]) 
	{
		save_flags(flags); cli();
		rs_interrupt_single(0, NULL, NULL);
		restore_flags(flags);
		mod_timer(&serial_timer, jiffies + IRQ_timeout[0]);
	}
}

/*
 * ---------------------------------------------------------------
 * Low level utility subroutines for the serial driver:  routines to
 * figure out the appropriate timeout for an interrupt chain, routines
 * to initialize and startup a serial port, and routines to shutdown a
 * serial port.  Useful stuff like that.
 * ---------------------------------------------------------------
 */

/*
 * This routine figures out the correct timeout for a particular IRQ.
 * It uses the smallest timeout of all of the serial ports in a
 * particular interrupt chain.  Now only used for IRQ 0....
 */
static void figure_IRQ_timeout(int irq)
{
	struct	async_struct	*info;
	int	timeout = 60*HZ;	/* 60 seconds === a long time :-) */

	info = IRQ_ports[irq];
	if (!info) {
		IRQ_timeout[irq] = 60*HZ;
		return;
	}
	while (info) {
		if (info->timeout < timeout)
			timeout = info->timeout;
		info = info->next_port;
	}
	if (!irq)
		timeout = timeout / 2;
	IRQ_timeout[irq] = (timeout > 3) ? timeout-2 : 1;
}


static int startup(struct async_struct * info)
{
	unsigned long flags;
	int	retval=0;
	void (*handler)(int, void *, struct pt_regs *);
	struct serial_state *state= info->state;
	unsigned long page;
//printk("startup\r\n");
	page = get_zeroed_page(GFP_KERNEL);
	if (!page)
		return -ENOMEM;

	save_flags(flags); cli();

	if (info->flags & ASYNC_INITIALIZED) {
		free_page(page);
		goto errout;
	}

	if (!CONFIGURED_SERIAL_PORT(state) || !state->type) 
	{
		if (info->tty)
			set_bit(TTY_IO_ERROR, &info->tty->flags);
		free_page(page);
		goto errout;
	}
	if (info->xmit.buf)
		free_page(page);
	else
		info->xmit.buf = (unsigned char *) page;

#ifdef SERIAL_DEBUG_OPEN
	printk("starting up ttys%d (irq %d)...", info->line, state->irq);
#endif



	/*
	 * Clear the FIFO buffers and disable them
	 * (they will be reenabled in change_speed())
	 */
	if (uart_config[state->type].flags & UART_CLEAR_FIFO) {
		serial_outp(info, SERIAL_FCR, SERIAL_FCR_FE);
		serial_outp(info, SERIAL_FCR, (SERIAL_FCR_FE |
					     SERIAL_FCR_RXFR |
					     SERIAL_FCR_TXFR));
		serial_outp(info, SERIAL_FCR, 0);
	}

	/*
	 * Clear the interrupt registers.
	 */
	(void) serial_inp(info, SERIAL_LSR);
	(void) serial_inp(info, SERIAL_RBR);
	(void) serial_inp(info, SERIAL_IIR);
	(void) serial_inp(info, SERIAL_MSR);

	/*
	 * At this point there's no way the LSR could still be 0xFF;
	 * if it is, then bail out, because there's likely no UART
	 * here.
	 */
	if (!(info->flags & ASYNC_BUGGY_UART) && (serial_inp(info, SERIAL_LSR) == 0xff)) 
	{
		printk("ttyS%d: LSR safety check engaged!\n", state->line);
		if (capable(CAP_SYS_ADMIN)) {
			if (info->tty)
				set_bit(TTY_IO_ERROR, &info->tty->flags);
		} else
			retval = -ENODEV;
		goto errout;
	}
	
	/*
	 * Allocate the IRQ if necessary
	 */
	if (state->irq && (!IRQ_ports[state->irq] || !IRQ_ports[state->irq]->next_port)) 
	{
		if (IRQ_ports[state->irq]) 
		{
#ifdef CONFIG_SERIAL_SHARE_IRQ
			free_irq(state->irq, &IRQ_ports[state->irq]);
				handler = rs_interrupt;
#else
			retval = -EBUSY;
			goto errout;
#endif /* CONFIG_SERIAL_SHARE_IRQ */
		} 
		else 
		{
			handler = rs_interrupt_single;
		}
//		printk("IRQ_ports = %x\n", IRQ_ports);
		cpe_int_set_irq(state->irq, LEVEL, H_ACTIVE);
		if(state->irq <32) //irq			
		{
			if(show_once++==1)
		  printk("IRQ UART at interrupt number 0x%x\r\n",state->irq);
		  retval = request_irq(state->irq, handler, 0,
				  "serial", &IRQ_ports[state->irq]);
		}
		else
		{
			if(show_once++==1)
		  printk("FIQ UART at interrupt number 0x%x\r\n",state->irq);
		  retval = request_irq(state->irq, handler, SA_INTERRUPT, 
				  "serial", &IRQ_ports[state->irq]);
		}
					
		if (retval) {
			if (capable(CAP_SYS_ADMIN)) {
				if (info->tty)
					set_bit(TTY_IO_ERROR,
						&info->tty->flags);
				retval = 0;
			}
			goto errout;
		}
	}

	/*
	 * Insert serial port into IRQ chain.
	 */
	info->prev_port = 0;
	info->next_port = IRQ_ports[state->irq];
	if (info->next_port)
		info->next_port->prev_port = info;
	IRQ_ports[state->irq] = info;
	figure_IRQ_timeout(state->irq);

	/*
	 * Now, initialize the UART 
	 */
	serial_outp(info, SERIAL_LCR, SERIAL_LCR_LEN8);	/* reset DLAB */

	info->MCR = 0;
	if (info->tty->termios->c_cflag & CBAUD)
		info->MCR = SERIAL_MCR_DTR | SERIAL_MCR_RTS;
	{
		if (state->irq != 0)
			info->MCR |= SERIAL_MCR_OUT2;
	}
	info->MCR |= ALPHA_KLUDGE_MCR; 		/* Don't ask */
	serial_outp(info, SERIAL_MCR, info->MCR);
	
	/*
	 * Finally, enable interrupts
	 */
	info->IER = SERIAL_IER_MS | SERIAL_IER_RLS | SERIAL_IER_DR;
	serial_outp(info, SERIAL_IER, info->IER);	/* enable interrupts */
	

	/*
	 * And clear the interrupt registers again for luck.
	 */
	(void)serial_inp(info, SERIAL_LSR);
	(void)serial_inp(info, SERIAL_RBR);
	(void)serial_inp(info, SERIAL_IIR);
	(void)serial_inp(info, SERIAL_MSR);

	if (info->tty)
		clear_bit(TTY_IO_ERROR, &info->tty->flags);
	info->xmit.head = info->xmit.tail = 0;

	/*
	 * Set up serial timers...
	 */
	mod_timer(&serial_timer, jiffies + 2*HZ/100);

	/*
	 * Set up the tty->alt_speed kludge
	 */
#if (LINUX_VERSION_CODE >= 131394) /* Linux 2.1.66 */
	if (info->tty) {
		if ((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_HI)
			info->tty->alt_speed = 57600;
		if ((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_VHI)
			info->tty->alt_speed = 115200;
		if ((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_SHI)
			info->tty->alt_speed = 230400;
		if ((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_WARP)
			info->tty->alt_speed = 460800;
	}
#endif
	
	/*
	 * and set the speed of the serial port
	 */
	change_speed(info, 0);

	info->flags |= ASYNC_INITIALIZED;
	restore_flags(flags);
	return 0;
	
errout:
	restore_flags(flags);
	return retval;
}



/*
 * This routine will shutdown a serial port; interrupts are disabled, and
 * DTR is dropped if the hangup on close termio flag is on.
 */
static void shutdown(struct async_struct * info)
{
	unsigned long	flags;
	struct serial_state *state;
	int		retval;
//printk("shutdown\r\n");
	if (!(info->flags & ASYNC_INITIALIZED))
		return;

	state = info->state;

#ifdef SERIAL_DEBUG_OPEN
	printk("Shutting down serial port %d (irq %d)....", info->line,
	       state->irq);
#endif
	
	save_flags(flags); cli(); /* Disable interrupts */

	/*
	 * clear delta_msr_wait queue to avoid mem leaks: we may free the irq
	 * here so the queue might never be waken up
	 */
	wake_up_interruptible(&info->delta_msr_wait);
	
	/*
	 * First unlink the serial port from the IRQ chain...
	 */
	if (info->next_port)
		info->next_port->prev_port = info->prev_port;
	if (info->prev_port)
		info->prev_port->next_port = info->next_port;
	else
		IRQ_ports[state->irq] = info->next_port;
	figure_IRQ_timeout(state->irq);
	
	/*
	 * Free the IRQ, if necessary
	 */
	if (state->irq && (!IRQ_ports[state->irq] ||
			  !IRQ_ports[state->irq]->next_port)) {
		if (IRQ_ports[state->irq]) {
			free_irq(state->irq, &IRQ_ports[state->irq]);
			retval = request_irq(state->irq, rs_interrupt_single,
					     SA_SHIRQ, "serial",
					     &IRQ_ports[state->irq]);
			
			if (retval)
				printk("serial shutdown: request_irq: error %d"
				       "  Couldn't reacquire IRQ.\n", retval);
		} else
			free_irq(state->irq, &IRQ_ports[state->irq]);
	}

	if (info->xmit.buf) {
		unsigned long pg = (unsigned long) info->xmit.buf;
		info->xmit.buf = 0;
		free_page(pg);
	}

	info->IER = 0;
	serial_outp(info, SERIAL_IER, 0x00);	/* disable all intrs */

	info->MCR &= ~SERIAL_MCR_OUT2;
	info->MCR |= ALPHA_KLUDGE_MCR; 		/* Don't ask */
	
	/* disable break condition */
	serial_out(info, SERIAL_LCR, serial_inp(info, SERIAL_LCR) & ~SERIAL_LCR_SETBREAK);
	
	if (!info->tty || (info->tty->termios->c_cflag & HUPCL))
		info->MCR &= ~(SERIAL_MCR_DTR|SERIAL_MCR_RTS);
	serial_outp(info, SERIAL_MCR, info->MCR);

	/* disable FIFO's */	
	serial_outp(info, SERIAL_FCR, (SERIAL_FCR_FE |
				     SERIAL_FCR_RXFR |
				     SERIAL_FCR_TXFR));
	serial_outp(info, SERIAL_FCR, 0);

	

	(void)serial_in(info, SERIAL_RBR);    /* read data port to reset things */
	
	if (info->tty)
		set_bit(TTY_IO_ERROR, &info->tty->flags);

#ifdef not_complete_yet
	if (uart_config[info->state->type].flags & UART_STARTECH) {
		/* Arrange to enter sleep mode */
		serial_outp(info, SERIAL_LCR, 0xBF);
		serial_outp(info, SERIAL_EFR, SERIAL_EFR_ECB);
		serial_outp(info, SERIAL_LCR, 0);
		serial_outp(info, SERIAL_IER, SERIAL_IERX_SLEEP);
		serial_outp(info, SERIAL_LCR, 0xBF);
		serial_outp(info, SERIAL_EFR, 0);
		serial_outp(info, SERIAL_LCR, 0);
	}
#endif /* end_of_not */

#ifdef not_complete_yet
	if (info->state->type == PORT_16750) {
		/* Arrange to enter sleep mode */
		serial_outp(info, SERIAL_IER, SERIAL_IERX_SLEEP);
	}
#endif /* end_of_not */
	info->flags &= ~ASYNC_INITIALIZED;
	restore_flags(flags);
}

#if (LINUX_VERSION_CODE < 131394) /* Linux 2.1.66 */
static int baud_table[] = {
	0, 50, 75, 110, 134, 150, 200, 300,
	600, 1200, 1800, 2400, 4800, 9600, 19200,
	38400, 57600, 115200, 230400, 460800, 0 };

static int tty_get_baud_rate(struct tty_struct *tty)
{
	struct async_struct * info = (struct async_struct *)tty->driver_data;
	unsigned int cflag, i;

	cflag = tty->termios->c_cflag;

	i = cflag & CBAUD;
	if (i & CBAUDEX) {
		i &= ~CBAUDEX;
		if (i < 1 || i > 2) 
			tty->termios->c_cflag &= ~CBAUDEX;
		else
			i += 15;
	}
	if (i == 15) {
		if ((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_HI)
			i += 1;
		if ((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_VHI)
			i += 2;
	}
	return baud_table[i];
}
#endif

/*
 * This routine is called to set the UART divisor registers to match
 * the specified baud rate for a serial port.
 */
static void change_speed(struct async_struct *info,
			 struct termios *old_termios)
{
	int	quot = 0, baud_base, baud;
	unsigned cflag, cval, fcr = 0;
	int	bits;
	unsigned long	flags;
//printk("change_speed\r\n");
	if (!info->tty || !info->tty->termios)
		return;
	cflag = info->tty->termios->c_cflag;
	if (!CONFIGURED_SERIAL_PORT(info))
		return;

	/* byte size and parity */
	switch (cflag & CSIZE) 
	{
	      case CS5: cval = 0x00; bits = 7; break;
	      case CS6: cval = 0x01; bits = 8; break;
	      case CS7: cval = 0x02; bits = 9; break;
	      case CS8: cval = 0x03; bits = 10; break;
	      /* Never happens, but GCC is too dumb to figure it out */
	      default:  cval = 0x00; bits = 7; break;
	}
	if (cflag & CSTOPB) {
		cval |= 0x04;
		bits++;
	}
	if (cflag & PARENB) {
		cval |= SERIAL_LCR_PE;
		bits++;
	}
	if (!(cflag & PARODD))
		cval |= SERIAL_LCR_EVEN;
#ifdef CMSPAR
	if (cflag & CMSPAR)
		cval |= SERIAL_LCR_STICKPARITY;
#endif

	/* Determine divisor based on baud rate */
	baud = tty_get_baud_rate(info->tty);
	if (!baud)
		baud = 9600;	/* B0 transition handled in rs_set_termios */

	baud_base = info->state->baud_base;

	if (baud == 38400 &&
	    ((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_CUST))
		quot = info->state->custom_divisor;
	else {
		if (baud == 134)
			/* Special case since 134 is really 134.5 */
			quot = (2*baud_base / 269);
		else if (baud)
			quot = baud_base / baud;
	}
	/* If the quotient is zero refuse the change */
	if (!quot && old_termios) {
		info->tty->termios->c_cflag &= ~CBAUD;
		info->tty->termios->c_cflag |= (old_termios->c_cflag & CBAUD);
		baud = tty_get_baud_rate(info->tty);
		if (!baud)
			baud = 9600;
		if (baud == 38400 &&
		    ((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_CUST))
			quot = info->state->custom_divisor;
		else {
			if (baud == 134)
				/* Special case since 134 is really 134.5 */
				quot = (2*baud_base / 269);
			else if (baud)
				quot = baud_base / baud;
		}
	}
	/* As a last resort, if the quotient is zero, default to 9600 bps */
	if (!quot)
		quot = baud_base / 9600;
	/*
	 * Work around a bug in the Oxford Semiconductor 952 rev B
	 * chip which causes it to seriously miscalculate baud rates
	 * when DLL is 0.
	 */
	if (((quot & 0xFF) == 0) && (info->state->type == PORT_16C950) &&
	    (info->state->revision == 0x5201))
		quot++;
	
	info->quot = quot;
	info->timeout = ((info->xmit_fifo_size*HZ*bits*quot) / baud_base);
	info->timeout += HZ/50;		/* Add .02 seconds of slop */

	/* Set up FIFO's */
	if (uart_config[info->state->type].flags & UART_USE_FIFO) 
	{
		if ((info->state->baud_base / quot) < 2400)
			fcr = SERIAL_FCR_FE | SERIAL_FCR_TRIGGER_1;

		else
			//ivan
			//fcr = SERIAL_FCR_FE | SERIAL_FCR_TRIGGER_8;
			fcr = SERIAL_FCR_FE | SERIAL_FCR_TRIGGER_14;
	}

	/* CTS flow control flag and modem status interrupts */
	info->IER &= ~SERIAL_IER_MS;
	if (info->flags & ASYNC_HARDPPS_CD)
		info->IER |= SERIAL_IER_MS;
	if (cflag & CRTSCTS) {
		info->flags |= ASYNC_CTS_FLOW;
		info->IER |= SERIAL_IER_MS;
	} else
		info->flags &= ~ASYNC_CTS_FLOW;
	if (cflag & CLOCAL)
		info->flags &= ~ASYNC_CHECK_CD;
	else {
		info->flags |= ASYNC_CHECK_CD;
		info->IER |= SERIAL_IER_MS;
	}
	serial_out(info, SERIAL_IER, info->IER);

	/*
	 * Set up parity check flag
	 */
#define RELEVANT_IFLAG(iflag) (iflag & (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK))

	info->read_status_mask = SERIAL_LSR_OE | SERIAL_LSR_THRE | SERIAL_LSR_DR;
	if (I_INPCK(info->tty))
		info->read_status_mask |= SERIAL_LSR_FE | SERIAL_LSR_PE;
	if (I_BRKINT(info->tty) || I_PARMRK(info->tty))
		info->read_status_mask |= SERIAL_LSR_BI;
	
	/*
	 * Characters to ignore
	 */
	info->ignore_status_mask = 0;
	if (I_IGNPAR(info->tty))
		info->ignore_status_mask |= SERIAL_LSR_PE | SERIAL_LSR_FE;
	if (I_IGNBRK(info->tty)) {
		info->ignore_status_mask |= SERIAL_LSR_BI;
		/*
		 * If we're ignore parity and break indicators, ignore 
		 * overruns too.  (For real raw support).
		 */
		if (I_IGNPAR(info->tty))
			info->ignore_status_mask |= SERIAL_LSR_OE;
	}
	/*
	 * !!! ignore all characters if CREAD is not set
	 */
	if ((cflag & CREAD) == 0)
		info->ignore_status_mask |= SERIAL_LSR_DR;
	save_flags(flags); cli();
	
	serial_outp(info, SERIAL_LCR, cval | SERIAL_LCR_DLAB);	/* set DLAB */
	serial_outp(info, SERIAL_DLL, quot & 0xff);				/* LS of divisor */
	serial_outp(info, SERIAL_DLM, quot >> 8);				/* MS of divisor */
	if (info->state->type == PORT_16750)
		serial_outp(info, SERIAL_FCR, fcr); 				/* set fcr */
	serial_outp(info, SERIAL_LCR, cval);					/* reset DLAB */
	info->LCR = cval;										/* Save LCR */
 	if (info->state->type != PORT_16750) 
 	{
 		if (fcr & SERIAL_FCR_FE) {
 			/* emulated UARTs (Lucent Venus 167x) need two steps */
 			serial_outp(info, SERIAL_FCR, SERIAL_FCR_FE);
 		}
		serial_outp(info, SERIAL_FCR, fcr); 	/* set fcr */
	}
	restore_flags(flags);
}

static void rs_put_char(struct tty_struct *tty, unsigned char ch)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	unsigned long flags;

	if (serial_paranoia_check(info, tty->device, "rs_put_char"))
		return;

	if (!tty || !info->xmit.buf)
		return;

	save_flags(flags); cli();
	if (CIRC_SPACE(info->xmit.head,
		       info->xmit.tail,
		       SERIAL_XMIT_SIZE) == 0) {
		restore_flags(flags);
		return;
	}

	info->xmit.buf[info->xmit.head] = ch;
	info->xmit.head = (info->xmit.head + 1) & (SERIAL_XMIT_SIZE-1);
	restore_flags(flags);
}

static void rs_flush_chars(struct tty_struct *tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	unsigned long flags;
				
	if (serial_paranoia_check(info, tty->device, "rs_flush_chars"))
		return;

	if (info->xmit.head == info->xmit.tail
	    || tty->stopped
	    || tty->hw_stopped
	    || !info->xmit.buf)
		return;

	save_flags(flags); cli();
	info->IER |= SERIAL_IER_TE;
	serial_out(info, SERIAL_IER, info->IER);
	restore_flags(flags);
}

static int rs_write(struct tty_struct * tty, int from_user, const unsigned char *buf, int count)
{
	int	c, ret = 0;
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	unsigned long flags;
				
	if (serial_paranoia_check(info, tty->device, "rs_write"))
		return 0;

	if (!tty || !info->xmit.buf || !tmp_buf)
		return 0;
	
//	buf[count] = '\0';
//	printk("rs_write: %s\n", buf);
	save_flags(flags);
	if (from_user)
	{
		down(&tmp_buf_sem);
		while (1) {
			int c1;
			c = CIRC_SPACE_TO_END(info->xmit.head,
					      info->xmit.tail,
					      SERIAL_XMIT_SIZE);
			if (count < c)
				c = count;
			if (c <= 0)
				break;

			c -= copy_from_user(tmp_buf, buf, c);
			if (!c) {
				if (!ret)
					ret = -EFAULT;
				break;
			}
			cli();
			c1 = CIRC_SPACE_TO_END(info->xmit.head,
					       info->xmit.tail,
					       SERIAL_XMIT_SIZE);
			if (c1 < c)
				c = c1;
			memcpy(info->xmit.buf + info->xmit.head, tmp_buf, c);
			info->xmit.head = ((info->xmit.head + c) &
					   (SERIAL_XMIT_SIZE-1));
			restore_flags(flags);
			buf += c;
			count -= c;
			ret += c;
		}
		up(&tmp_buf_sem);
	} 
	else 
	{
		cli();
		while (1) 
		{
			c = CIRC_SPACE_TO_END(info->xmit.head,
					      info->xmit.tail,
					      SERIAL_XMIT_SIZE);
			if (count < c)
				c = count;
			if (c <= 0) {
				break;
			}
			memcpy(info->xmit.buf + info->xmit.head, buf, c);
			info->xmit.head = ((info->xmit.head + c) &
					   (SERIAL_XMIT_SIZE-1));
			buf += c;
			count -= c;
			ret += c;
		}
		restore_flags(flags);
	}
	if (info->xmit.head != info->xmit.tail
	    && !tty->stopped
	    && !tty->hw_stopped
	    && !(info->IER & SERIAL_IER_TE)) 
	{
		info->IER |= SERIAL_IER_TE;
		serial_out(info, SERIAL_IER, info->IER);
	}
	return ret;
}

static int rs_write_room(struct tty_struct *tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rs_write_room"))
		return 0;
	return CIRC_SPACE(info->xmit.head, info->xmit.tail, SERIAL_XMIT_SIZE);
}

static int rs_chars_in_buffer(struct tty_struct *tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
				
	if (serial_paranoia_check(info, tty->device, "rs_chars_in_buffer"))
		return 0;
	return CIRC_CNT(info->xmit.head, info->xmit.tail, SERIAL_XMIT_SIZE);
}

static void rs_flush_buffer(struct tty_struct *tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	unsigned long flags;
	
	if (serial_paranoia_check(info, tty->device, "rs_flush_buffer"))
		return;
	save_flags(flags); cli();
	info->xmit.head = info->xmit.tail = 0;
	restore_flags(flags);
	wake_up_interruptible(&tty->write_wait);
#ifdef SERIAL_HAVE_POLL_WAIT
	wake_up_interruptible(&tty->poll_wait);
#endif
	if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
	    tty->ldisc.write_wakeup)
		(tty->ldisc.write_wakeup)(tty);
}

/*
 * This function is used to send a high-priority XON/XOFF character to
 * the device
 */
static void rs_send_xchar(struct tty_struct *tty, char ch)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;

	if (serial_paranoia_check(info, tty->device, "rs_send_char"))
		return;

	info->x_char = ch;
	if (ch) {
		/* Make sure transmit interrupts are on */
		info->IER |= SERIAL_IER_TE;
		serial_out(info, SERIAL_IER, info->IER);
	}
}

/*
 * ------------------------------------------------------------
 * rs_throttle()
 * 
 * This routine is called by the upper-layer tty layer to signal that
 * incoming characters should be throttled.
 * ------------------------------------------------------------
 */
static void rs_throttle(struct tty_struct * tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	unsigned long flags;
#ifdef SERIAL_DEBUG_THROTTLE
	char	buf[64];
	
	printk("throttle %s: %d....\n", tty_name(tty, buf),
	       tty->ldisc.chars_in_buffer(tty));
#endif

	if (serial_paranoia_check(info, tty->device, "rs_throttle"))
		return;
	
	if (I_IXOFF(tty))
		rs_send_xchar(tty, STOP_CHAR(tty));

	if (tty->termios->c_cflag & CRTSCTS)
		info->MCR &= ~SERIAL_MCR_RTS;

	save_flags(flags); cli();
	serial_out(info, SERIAL_MCR, info->MCR);
	restore_flags(flags);
}

static void rs_unthrottle(struct tty_struct * tty)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	unsigned long flags;
#ifdef SERIAL_DEBUG_THROTTLE
	char	buf[64];
	
	printk("unthrottle %s: %d....\n", tty_name(tty, buf),
	       tty->ldisc.chars_in_buffer(tty));
#endif

	if (serial_paranoia_check(info, tty->device, "rs_unthrottle"))
		return;
	
	if (I_IXOFF(tty)) {
		if (info->x_char)
			info->x_char = 0;
		else
			rs_send_xchar(tty, START_CHAR(tty));
	}
	if (tty->termios->c_cflag & CRTSCTS)
		info->MCR |= SERIAL_MCR_RTS;
	save_flags(flags); cli();
	serial_out(info, SERIAL_MCR, info->MCR);
	restore_flags(flags);
}

/*
 * ------------------------------------------------------------
 * rs_ioctl() and friends
 * ------------------------------------------------------------
 */

static int get_serial_info(struct async_struct * info,
			   struct serial_struct * retinfo)
{
	struct serial_struct tmp;
	struct serial_state *state = info->state;
   
	if (!retinfo)
		return -EFAULT;
	memset(&tmp, 0, sizeof(tmp));
	tmp.type = state->type;
	tmp.line = state->line;
	tmp.port = state->port;
	if (HIGH_BITS_OFFSET)
		tmp.port_high = state->port >> HIGH_BITS_OFFSET;
	else
		tmp.port_high = 0;
	tmp.irq = state->irq;
	tmp.flags = state->flags;
	tmp.xmit_fifo_size = state->xmit_fifo_size;
	tmp.baud_base = state->baud_base;
	tmp.close_delay = state->close_delay;
	tmp.closing_wait = state->closing_wait;
	tmp.custom_divisor = state->custom_divisor;
	tmp.hub6 = state->hub6;
	tmp.io_type = state->io_type;
	if (copy_to_user(retinfo,&tmp,sizeof(*retinfo)))
		return -EFAULT;
	return 0;
}

static int set_serial_info(struct async_struct * info,
			   struct serial_struct * new_info)
{
	struct serial_struct new_serial;
 	struct serial_state old_state, *state;
	unsigned int		i,change_irq,change_port;
	int 			retval = 0;
	unsigned long		new_port;

	if (copy_from_user(&new_serial,new_info,sizeof(new_serial)))
		return -EFAULT;
	state = info->state;
	old_state = *state;

	new_port = new_serial.port;
	if (HIGH_BITS_OFFSET)
		new_port += (unsigned long) new_serial.port_high << HIGH_BITS_OFFSET;

	change_irq = new_serial.irq != state->irq;
	change_port = (new_port != ((int) state->port)) ||
		(new_serial.hub6 != state->hub6);
  
	if (!capable(CAP_SYS_ADMIN)) {
		if (change_irq || change_port ||
		    (new_serial.baud_base != state->baud_base) ||
		    (new_serial.type != state->type) ||
		    (new_serial.close_delay != state->close_delay) ||
		    (new_serial.xmit_fifo_size != state->xmit_fifo_size) ||
		    ((new_serial.flags & ~ASYNC_USR_MASK) !=
		     (state->flags & ~ASYNC_USR_MASK)))
			return -EPERM;
		state->flags = ((state->flags & ~ASYNC_USR_MASK) |
			       (new_serial.flags & ASYNC_USR_MASK));
		info->flags = ((info->flags & ~ASYNC_USR_MASK) |
			       (new_serial.flags & ASYNC_USR_MASK));
		state->custom_divisor = new_serial.custom_divisor;
		goto check_and_exit;
	}

	new_serial.irq = irq_cannonicalize(new_serial.irq);

	if ((new_serial.irq >= NR_IRQS) || (new_serial.irq < 0) || 
	    (new_serial.baud_base < 9600)|| (new_serial.type < PORT_UNKNOWN) ||
	    (new_serial.type > PORT_MAX) || (new_serial.type == PORT_CIRRUS) ||
	    (new_serial.type == PORT_STARTECH)) {
		return -EINVAL;
	}

	if ((new_serial.type != state->type) ||
	    (new_serial.xmit_fifo_size <= 0))
		new_serial.xmit_fifo_size =
			uart_config[new_serial.type].dfl_xmit_fifo_size;

	/* Make sure address is not already in use */
	if (new_serial.type) {
		for (i = 0 ; i < NR_PORTS; i++)
			if ((state != &rs_table[i]) &&
			    (rs_table[i].port == new_port) &&
			    rs_table[i].type)
				return -EADDRINUSE;
	}

	if ((change_port || change_irq) && (state->count > 1))
		return -EBUSY;

	/*
	 * OK, past this point, all the error checking has been done.
	 * At this point, we start making changes.....
	 */

	state->baud_base = new_serial.baud_base;
	state->flags = ((state->flags & ~ASYNC_FLAGS) |
			(new_serial.flags & ASYNC_FLAGS));
	info->flags = ((state->flags & ~ASYNC_INTERNAL_FLAGS) |
		       (info->flags & ASYNC_INTERNAL_FLAGS));
	state->custom_divisor = new_serial.custom_divisor;
	state->close_delay = new_serial.close_delay * HZ/100;
	state->closing_wait = new_serial.closing_wait * HZ/100;
#if (LINUX_VERSION_CODE > 0x20100)
	info->tty->low_latency = (info->flags & ASYNC_LOW_LATENCY) ? 1 : 0;
#endif
	info->xmit_fifo_size = state->xmit_fifo_size =
		new_serial.xmit_fifo_size;

	if ((state->type != PORT_UNKNOWN) && state->port) {

		release_region(state->port,8);
	}
	state->type = new_serial.type;
	if (change_port || change_irq) {
		/*
		 * We need to shutdown the serial port at the old
		 * port/irq combination.
		 */
		shutdown(info);
		state->irq = new_serial.irq;
		info->port = state->port = new_port;
		info->hub6 = state->hub6 = new_serial.hub6;
		if (info->hub6)
			info->io_type = state->io_type = SERIAL_IO_HUB6;
		else if (info->io_type == SERIAL_IO_HUB6)
			info->io_type = state->io_type = SERIAL_IO_PORT;
	}
	if ((state->type != PORT_UNKNOWN) && state->port) {
			request_region(state->port,8,"serial(set)");
	}

	
check_and_exit:
	if (!state->port || !state->type)
		return 0;
	if (info->flags & ASYNC_INITIALIZED) {
		if (((old_state.flags & ASYNC_SPD_MASK) !=
		     (state->flags & ASYNC_SPD_MASK)) ||
		    (old_state.custom_divisor != state->custom_divisor)) {
#if (LINUX_VERSION_CODE >= 131394) /* Linux 2.1.66 */
			if ((state->flags & ASYNC_SPD_MASK) == ASYNC_SPD_HI)
				info->tty->alt_speed = 57600;
			if ((state->flags & ASYNC_SPD_MASK) == ASYNC_SPD_VHI)
				info->tty->alt_speed = 115200;
			if ((state->flags & ASYNC_SPD_MASK) == ASYNC_SPD_SHI)
				info->tty->alt_speed = 230400;
			if ((state->flags & ASYNC_SPD_MASK) == ASYNC_SPD_WARP)
				info->tty->alt_speed = 460800;
#endif
			change_speed(info, 0);
		}
	} else
		retval = startup(info);
	return retval;
}


/*
 * get_lsr_info - get line status register info
 *
 * Purpose: Let user call ioctl() to get info when the UART physically
 * 	    is emptied.  On bus types like RS485, the transmitter must
 * 	    release the bus after transmitting. This must be done when
 * 	    the transmit shift register is empty, not be done when the
 * 	    transmit holding register is empty.  This functionality
 * 	    allows an RS485 driver to be written in user space. 
 */
static int get_lsr_info(struct async_struct * info, unsigned int *value)
{
	unsigned char status;
	unsigned int result;
	unsigned long flags;

	save_flags(flags); cli();
	status = serial_in(info, SERIAL_LSR);
	restore_flags(flags);
	result = ((status & SERIAL_LSR_TE) ? TIOCSER_TEMT : 0);

	/*
	 * If we're about to load something into the transmit
	 * register, we'll pretend the transmitter isn't empty to
	 * avoid a race condition (depending on when the transmit
	 * interrupt happens).
	 */
	if (info->x_char || 
	    ((CIRC_CNT(info->xmit.head, info->xmit.tail,
		       SERIAL_XMIT_SIZE) > 0) &&
	     !info->tty->stopped && !info->tty->hw_stopped))
		result &= ~TIOCSER_TEMT;

	if (copy_to_user(value, &result, sizeof(int)))
		return -EFAULT;
	return 0;
}


static int get_modem_info(struct async_struct * info, unsigned int *value)
{
	unsigned char control, status;
	unsigned int result;
	unsigned long flags;

	control = info->MCR;
	save_flags(flags); cli();
	status = serial_in(info, SERIAL_MSR);
	restore_flags(flags);
	result =  ((control & SERIAL_MCR_RTS) ? TIOCM_RTS : 0)
		| ((control & SERIAL_MCR_DTR) ? TIOCM_DTR : 0)
#ifdef TIOCM_OUT1
		| ((control & SERIAL_MCR_OUT1) ? TIOCM_OUT1 : 0)
		| ((control & SERIAL_MCR_OUT2) ? TIOCM_OUT2 : 0)
#endif
		| ((status  & SERIAL_MSR_DCD) ? TIOCM_CAR : 0)
		| ((status  & SERIAL_MSR_RI) ? TIOCM_RNG : 0)
		| ((status  & SERIAL_MSR_DSR) ? TIOCM_DSR : 0)
		| ((status  & SERIAL_MSR_CTS) ? TIOCM_CTS : 0);

	if (copy_to_user(value, &result, sizeof(int)))
		return -EFAULT;
	return 0;
}

static int set_modem_info(struct async_struct * info, unsigned int cmd,
			  unsigned int *value)
{
	unsigned int arg;
	unsigned long flags;

	if (copy_from_user(&arg, value, sizeof(int)))
		return -EFAULT;

	switch (cmd) {
	case TIOCMBIS: 
		if (arg & TIOCM_RTS)
			info->MCR |= SERIAL_MCR_RTS;
		if (arg & TIOCM_DTR)
			info->MCR |= SERIAL_MCR_DTR;
#ifdef TIOCM_OUT1
		if (arg & TIOCM_OUT1)
			info->MCR |= SERIAL_MCR_OUT1;
		if (arg & TIOCM_OUT2)
			info->MCR |= SERIAL_MCR_OUT2;
#endif
		if (arg & TIOCM_LOOP)
			info->MCR |= SERIAL_MCR_LPBK;
		break;
	case TIOCMBIC:
		if (arg & TIOCM_RTS)
			info->MCR &= ~SERIAL_MCR_RTS;
		if (arg & TIOCM_DTR)
			info->MCR &= ~SERIAL_MCR_DTR;
#ifdef TIOCM_OUT1
		if (arg & TIOCM_OUT1)
			info->MCR &= ~SERIAL_MCR_OUT1;
		if (arg & TIOCM_OUT2)
			info->MCR &= ~SERIAL_MCR_OUT2;
#endif
		if (arg & TIOCM_LOOP)
			info->MCR &= ~SERIAL_MCR_LPBK;
		break;
	case TIOCMSET:
		info->MCR = ((info->MCR & ~(SERIAL_MCR_RTS |
#ifdef TIOCM_OUT1
					    SERIAL_MCR_OUT1 |
					    SERIAL_MCR_OUT2 |
#endif
					    SERIAL_MCR_LPBK |
					    SERIAL_MCR_DTR))
			     | ((arg & TIOCM_RTS) ? SERIAL_MCR_RTS : 0)
#ifdef TIOCM_OUT1
			     | ((arg & TIOCM_OUT1) ? SERIAL_MCR_OUT1 : 0)
			     | ((arg & TIOCM_OUT2) ? SERIAL_MCR_OUT2 : 0)
#endif
			     | ((arg & TIOCM_LOOP) ? SERIAL_MCR_LPBK : 0)
			     | ((arg & TIOCM_DTR) ? SERIAL_MCR_DTR : 0));
		break;
	default:
		return -EINVAL;
	}
	save_flags(flags); cli();
	info->MCR |= ALPHA_KLUDGE_MCR; 		/* Don't ask */
	serial_out(info, SERIAL_MCR, info->MCR);
	restore_flags(flags);
	return 0;
}

static int do_autoconfig(struct async_struct * info)
{
	int irq, retval;
	
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	
	if (info->state->count > 1)
		return -EBUSY;
	
	shutdown(info);

	autoconfig(info->state);
	if ((info->state->flags & ASYNC_AUTO_IRQ) &&
	    (info->state->port != 0  || info->state->iomem_base != 0) &&
	    (info->state->type != PORT_UNKNOWN)) {
		irq = detect_uart_irq(info->state);
		if (irq > 0)
			info->state->irq = irq;
	}

	retval = startup(info);
	if (retval)
		return retval;
	return 0;
}

/*
 * rs_break() --- routine which turns the break handling on or off
 */
#if (LINUX_VERSION_CODE < 131394) /* Linux 2.1.66 */
static void send_break(	struct async_struct * info, int duration)
{
	if (!CONFIGURED_SERIAL_PORT(info))
		return;
	current->state = TASK_INTERRUPTIBLE;
	current->timeout = jiffies + duration;
	cli();
	info->LCR |= SERIAL_LCR_SETBREAK;
	serial_out(info, SERIAL_LCR, info->LCR);
	schedule();
	info->LCR &= ~SERIAL_LCR_SETBREAK;
	serial_out(info, SERIAL_LCR, info->LCR);
	sti();
}
#else
static void rs_break(struct tty_struct *tty, int break_state)
{
	struct async_struct * info = (struct async_struct *)tty->driver_data;
	unsigned long flags;
	
	if (serial_paranoia_check(info, tty->device, "rs_break"))
		return;

	if (!CONFIGURED_SERIAL_PORT(info))
		return;
	save_flags(flags); cli();
	if (break_state == -1)
		info->LCR |= SERIAL_LCR_SETBREAK;
	else
		info->LCR &= ~SERIAL_LCR_SETBREAK;
	serial_out(info, SERIAL_LCR, info->LCR);
	restore_flags(flags);
}
#endif


static int rs_ioctl(struct tty_struct *tty, struct file * file,
		    unsigned int cmd, unsigned long arg)
{
	struct async_struct * info = (struct async_struct *)tty->driver_data;
	struct async_icount cprev, cnow;	/* kernel counter temps */
	struct serial_icounter_struct icount;
	unsigned long flags;
#if (LINUX_VERSION_CODE < 131394) /* Linux 2.1.66 */
	int retval, tmp;
#endif
	
	if (serial_paranoia_check(info, tty->device, "rs_ioctl"))
		return -ENODEV;

	if ((cmd != TIOCGSERIAL) && (cmd != TIOCSSERIAL) &&
	    (cmd != TIOCSERCONFIG) && (cmd != TIOCSERGSTRUCT) &&
	    (cmd != TIOCMIWAIT) && (cmd != TIOCGICOUNT)) {
		if (tty->flags & (1 << TTY_IO_ERROR))
		    return -EIO;
	}
	
	switch (cmd) {
#if (LINUX_VERSION_CODE < 131394) /* Linux 2.1.66 */
		case TCSBRK:	/* SVID version: non-zero arg --> no break */
			retval = tty_check_change(tty);
			if (retval)
				return retval;
			tty_wait_until_sent(tty, 0);
			if (signal_pending(current))
				return -EINTR;
			if (!arg) {
				send_break(info, HZ/4);	/* 1/4 second */
				if (signal_pending(current))
					return -EINTR;
			}
			return 0;
		case TCSBRKP:	/* support for POSIX tcsendbreak() */
			retval = tty_check_change(tty);
			if (retval)
				return retval;
			tty_wait_until_sent(tty, 0);
			if (signal_pending(current))
				return -EINTR;
			send_break(info, arg ? arg*(HZ/10) : HZ/4);
			if (signal_pending(current))
				return -EINTR;
			return 0;
		case TIOCGSOFTCAR:
			tmp = C_CLOCAL(tty) ? 1 : 0;
			if (copy_to_user((void *)arg, &tmp, sizeof(int)))
				return -EFAULT;
			return 0;
		case TIOCSSOFTCAR:
			if (copy_from_user(&tmp, (void *)arg, sizeof(int)))
				return -EFAULT;

			tty->termios->c_cflag =
				((tty->termios->c_cflag & ~CLOCAL) |
				 (tmp ? CLOCAL : 0));
			return 0;
#endif
		case TIOCMGET:
			return get_modem_info(info, (unsigned int *) arg);
		case TIOCMBIS:
		case TIOCMBIC:
		case TIOCMSET:
			return set_modem_info(info, cmd, (unsigned int *) arg);
		case TIOCGSERIAL:
			return get_serial_info(info,
					       (struct serial_struct *) arg);
		case TIOCSSERIAL:
			return set_serial_info(info,
					       (struct serial_struct *) arg);
		case TIOCSERCONFIG:
			return do_autoconfig(info);

		case TIOCSERGETLSR: /* Get line status register */
			return get_lsr_info(info, (unsigned int *) arg);

		case TIOCSERGSTRUCT:
			if (copy_to_user((struct async_struct *) arg,
					 info, sizeof(struct async_struct)))
				return -EFAULT;
			return 0;
				
			
		/*
		 * Wait for any of the 4 modem inputs (DCD,RI,DSR,CTS) to change
		 * - mask passed in arg for lines of interest
 		 *   (use |'ed TIOCM_RNG/DSR/CD/CTS for masking)
		 * Caller should use TIOCGICOUNT to see which one it was
		 */
		case TIOCMIWAIT:
			save_flags(flags); cli();
			/* note the counters on entry */
			cprev = info->state->icount;
			restore_flags(flags);
			/* Force modem status interrupts on */
			info->IER |= SERIAL_IER_MS;
			serial_out(info, SERIAL_IER, info->IER);
			while (1) {
				interruptible_sleep_on(&info->delta_msr_wait);
				/* see if a signal did it */
				if (signal_pending(current))
					return -ERESTARTSYS;
				save_flags(flags); cli();
				cnow = info->state->icount; /* atomic copy */
				restore_flags(flags);
				if (cnow.rng == cprev.rng && cnow.dsr == cprev.dsr && 
				    cnow.dcd == cprev.dcd && cnow.cts == cprev.cts)
					return -EIO; /* no change => error */
				if ( ((arg & TIOCM_RNG) && (cnow.rng != cprev.rng)) ||
				     ((arg & TIOCM_DSR) && (cnow.dsr != cprev.dsr)) ||
				     ((arg & TIOCM_CD)  && (cnow.dcd != cprev.dcd)) ||
				     ((arg & TIOCM_CTS) && (cnow.cts != cprev.cts)) ) {
					return 0;
				}
				cprev = cnow;
			}
			/* NOTREACHED */

		/* 
		 * Get counter of input serial line interrupts (DCD,RI,DSR,CTS)
		 * Return: write counters to the user passed counter struct
		 * NB: both 1->0 and 0->1 transitions are counted except for
		 *     RI where only 0->1 is counted.
		 */
		case TIOCGICOUNT:
			save_flags(flags); cli();
			cnow = info->state->icount;
			restore_flags(flags);
			icount.cts = cnow.cts;
			icount.dsr = cnow.dsr;
			icount.rng = cnow.rng;
			icount.dcd = cnow.dcd;
			icount.rx = cnow.rx;
			icount.tx = cnow.tx;
			icount.frame = cnow.frame;
			icount.overrun = cnow.overrun;
			icount.parity = cnow.parity;
			icount.brk = cnow.brk;
			icount.buf_overrun = cnow.buf_overrun;
			
			if (copy_to_user((void *)arg, &icount, sizeof(icount)))
				return -EFAULT;
			return 0;
		case TIOCSERGWILD:
		case TIOCSERSWILD:
			/* "setserial -W" is called in Debian boot */
			printk ("TIOCSER?WILD ioctl obsolete, ignored.\n");
			return 0;

		default:
			return -ENOIOCTLCMD;
		}
	return 0;
}

static void rs_set_termios(struct tty_struct *tty, struct termios *old_termios)
{
	struct async_struct *info = (struct async_struct *)tty->driver_data;
	unsigned long flags;
	unsigned int cflag = tty->termios->c_cflag;
	
	if (   (cflag == old_termios->c_cflag)
	    && (   RELEVANT_IFLAG(tty->termios->c_iflag) 
		== RELEVANT_IFLAG(old_termios->c_iflag)))
	  return;

	change_speed(info, old_termios);

	/* Handle transition to B0 status */
	if ((old_termios->c_cflag & CBAUD) &&
	    !(cflag & CBAUD)) {
		info->MCR &= ~(SERIAL_MCR_DTR|SERIAL_MCR_RTS);
		save_flags(flags); cli();
		serial_out(info, SERIAL_MCR, info->MCR);
		restore_flags(flags);
	}
	
	/* Handle transition away from B0 status */
	if (!(old_termios->c_cflag & CBAUD) &&
	    (cflag & CBAUD)) {
		info->MCR |= SERIAL_MCR_DTR;
		if (!(tty->termios->c_cflag & CRTSCTS) || 
		    !test_bit(TTY_THROTTLED, &tty->flags)) {
			info->MCR |= SERIAL_MCR_RTS;
		}
		save_flags(flags); cli();
		serial_out(info, SERIAL_MCR, info->MCR);
		restore_flags(flags);
	}
	
	/* Handle turning off CRTSCTS */
	if ((old_termios->c_cflag & CRTSCTS) &&
	    !(tty->termios->c_cflag & CRTSCTS)) {
		tty->hw_stopped = 0;
		rs_start(tty);
	}

#if 0
	/*
	 * No need to wake up processes in open wait, since they
	 * sample the CLOCAL flag once, and don't recheck it.
	 * XXX  It's not clear whether the current behavior is correct
	 * or not.  Hence, this may change.....
	 */
	if (!(old_termios->c_cflag & CLOCAL) &&
	    (tty->termios->c_cflag & CLOCAL))
		wake_up_interruptible(&info->open_wait);
#endif
}

/*
 * ------------------------------------------------------------
 * rs_close()
 * 
 * This routine is called when the serial port gets closed.  First, we
 * wait for the last remaining data to be sent.  Then, we unlink its
 * async structure from the interrupt chain if necessary, and we free
 * that IRQ if nothing is left in the chain.
 * ------------------------------------------------------------
 */
static void rs_close(struct tty_struct *tty, struct file * filp)
{
	struct async_struct * info = (struct async_struct *)tty->driver_data;
	struct serial_state *state;
	unsigned long flags;

	if (!info || serial_paranoia_check(info, tty->device, "rs_close"))
		return;

	state = info->state;
	
	save_flags(flags); cli();
	
	if (tty_hung_up_p(filp)) {
		DBG_CNT("before DEC-hung");
		MOD_DEC_USE_COUNT;
		restore_flags(flags);
		return;
	}
	
#ifdef SERIAL_DEBUG_OPEN
	printk("rs_close ttys%d, count = %d\n", info->line, state->count);
#endif
	if ((tty->count == 1) && (state->count != 1)) {
		/*
		 * Uh, oh.  tty->count is 1, which means that the tty
		 * structure will be freed.  state->count should always
		 * be one in these conditions.  If it's greater than
		 * one, we've got real problems, since it means the
		 * serial port won't be shutdown.
		 */
		printk("rs_close: bad serial port count; tty->count is 1, "
		       "state->count is %d\n", state->count);
		state->count = 1;
	}
	if (--state->count < 0) {
		printk("rs_close: bad serial port count for ttys%d: %d\n",
		       info->line, state->count);
		state->count = 0;
	}
	if (state->count) {
		DBG_CNT("before DEC-2");
		MOD_DEC_USE_COUNT;
		restore_flags(flags);
		return;
	}
	info->flags |= ASYNC_CLOSING;
	restore_flags(flags);
	/*
	 * Save the termios structure, since this port may have
	 * separate termios for callout and dialin.
	 */
	if (info->flags & ASYNC_NORMAL_ACTIVE)
		info->state->normal_termios = *tty->termios;
	if (info->flags & ASYNC_CALLOUT_ACTIVE)
		info->state->callout_termios = *tty->termios;
	/*
	 * Now we wait for the transmit buffer to clear; and we notify 
	 * the line discipline to only process XON/XOFF characters.
	 */
	tty->closing = 1;
	if (info->closing_wait != ASYNC_CLOSING_WAIT_NONE)
		tty_wait_until_sent(tty, info->closing_wait);
	/*
	 * At this point we stop accepting input.  To do this, we
	 * disable the receive line status interrupts, and tell the
	 * interrupt driver to stop checking the data ready bit in the
	 * line status register.
	 */
	info->IER &= ~SERIAL_IER_RLS;
	info->read_status_mask &= ~SERIAL_LSR_DR;
	if (info->flags & ASYNC_INITIALIZED) {
		serial_out(info, SERIAL_IER, info->IER);
		/*
		 * Before we drop DTR, make sure the UART transmitter
		 * has completely drained; this is especially
		 * important if there is a transmit FIFO!
		 */
		rs_wait_until_sent(tty, info->timeout);
	}
	shutdown(info);
	if (tty->driver.flush_buffer)
		tty->driver.flush_buffer(tty);
	if (tty->ldisc.flush_buffer)
		tty->ldisc.flush_buffer(tty);
	tty->closing = 0;
	info->event = 0;
	info->tty = 0;
	if (info->blocked_open) {
		if (info->close_delay) {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(info->close_delay);
		}
		wake_up_interruptible(&info->open_wait);
	}
	info->flags &= ~(ASYNC_NORMAL_ACTIVE|ASYNC_CALLOUT_ACTIVE|
			 ASYNC_CLOSING);
	wake_up_interruptible(&info->close_wait);
	MOD_DEC_USE_COUNT;
}

/*
 * rs_wait_until_sent() --- wait until the transmitter is empty
 */
static void rs_wait_until_sent(struct tty_struct *tty, int timeout)
{
	struct async_struct * info = (struct async_struct *)tty->driver_data;
	unsigned long orig_jiffies, char_time;
	int lsr;
	
	if (serial_paranoia_check(info, tty->device, "rs_wait_until_sent"))
		return;

	if (info->state->type == PORT_UNKNOWN)
		return;

	if (info->xmit_fifo_size == 0)
		return; /* Just in case.... */

	orig_jiffies = jiffies;
	/*
	 * Set the check interval to be 1/5 of the estimated time to
	 * send a single character, and make it at least 1.  The check
	 * interval should also be less than the timeout.
	 * 
	 * Note: we have to use pretty tight timings here to satisfy
	 * the NIST-PCTS.
	 */
	char_time = (info->timeout - HZ/50) / info->xmit_fifo_size;
	char_time = char_time / 5;
	if (char_time == 0)
		char_time = 1;
	if (timeout && timeout < char_time)
		char_time = timeout;
	/*
	 * If the transmitter hasn't cleared in twice the approximate
	 * amount of time to send the entire FIFO, it probably won't
	 * ever clear.  This assumes the UART isn't doing flow
	 * control, which is currently the case.  Hence, if it ever
	 * takes longer than info->timeout, this is probably due to a
	 * UART bug of some kind.  So, we clamp the timeout parameter at
	 * 2*info->timeout.
	 */
	if (!timeout || timeout > 2*info->timeout)
		timeout = 2*info->timeout;
#ifdef SERIAL_DEBUG_RS_WAIT_UNTIL_SENT
	printk("In rs_wait_until_sent(%d) check=%lu...", timeout, char_time);
	printk("jiff=%lu...", jiffies);
#endif
	while (!((lsr = serial_inp(info, SERIAL_LSR)) & SERIAL_LSR_TE)) {
#ifdef SERIAL_DEBUG_RS_WAIT_UNTIL_SENT
		printk("lsr = %d (jiff=%lu)...", lsr, jiffies);
#endif
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(char_time);
		if (signal_pending(current))
			break;
		if (timeout && time_after(jiffies, orig_jiffies + timeout))
			break;
	}
#ifdef SERIAL_DEBUG_RS_WAIT_UNTIL_SENT
	printk("lsr = %d (jiff=%lu)...done\n", lsr, jiffies);
#endif
}

/*
 * rs_hangup() --- called by tty_hangup() when a hangup is signaled.
 */
static void rs_hangup(struct tty_struct *tty)
{
	struct async_struct * info = (struct async_struct *)tty->driver_data;
	struct serial_state *state = info->state;
	
	if (serial_paranoia_check(info, tty->device, "rs_hangup"))
		return;

	state = info->state;
	
	rs_flush_buffer(tty);
	if (info->flags & ASYNC_CLOSING)
		return;
	shutdown(info);
	info->event = 0;
	state->count = 0;
	info->flags &= ~(ASYNC_NORMAL_ACTIVE|ASYNC_CALLOUT_ACTIVE);
	info->tty = 0;
	wake_up_interruptible(&info->open_wait);
}

/*
 * ------------------------------------------------------------
 * rs_open() and friends
 * ------------------------------------------------------------
 */
static int block_til_ready(struct tty_struct *tty, struct file * filp,
			   struct async_struct *info)
{
	DECLARE_WAITQUEUE(wait, current);
	struct serial_state *state = info->state;
	int		retval;
	int		do_clocal = 0, extra_count = 0;
	unsigned long	flags;

	/*
	 * If the device is in the middle of being closed, then block
	 * until it's done, and then try again.
	 */
	if (tty_hung_up_p(filp) ||
	    (info->flags & ASYNC_CLOSING)) {
		if (info->flags & ASYNC_CLOSING)
			interruptible_sleep_on(&info->close_wait);
#ifdef SERIAL_DO_RESTART
		return ((info->flags & ASYNC_HUP_NOTIFY) ?
			-EAGAIN : -ERESTARTSYS);
#else
		return -EAGAIN;
#endif
	}

	/*
	 * If this is a callout device, then just make sure the normal
	 * device isn't being used.
	 */
	if (tty->driver.subtype == SERIAL_TYPE_CALLOUT) {
		if (info->flags & ASYNC_NORMAL_ACTIVE)
			return -EBUSY;
		if ((info->flags & ASYNC_CALLOUT_ACTIVE) &&
		    (info->flags & ASYNC_SESSION_LOCKOUT) &&
		    (info->session != current->session))
		    return -EBUSY;
		if ((info->flags & ASYNC_CALLOUT_ACTIVE) &&
		    (info->flags & ASYNC_PGRP_LOCKOUT) &&
		    (info->pgrp != current->pgrp))
		    return -EBUSY;
		info->flags |= ASYNC_CALLOUT_ACTIVE;
		return 0;
	}
	
	/*
	 * If non-blocking mode is set, or the port is not enabled,
	 * then make the check up front and then exit.
	 */
	if ((filp->f_flags & O_NONBLOCK) ||
	    (tty->flags & (1 << TTY_IO_ERROR))) {
		if (info->flags & ASYNC_CALLOUT_ACTIVE)
			return -EBUSY;
		info->flags |= ASYNC_NORMAL_ACTIVE;
		return 0;
	}

	if (info->flags & ASYNC_CALLOUT_ACTIVE) {
		if (state->normal_termios.c_cflag & CLOCAL)
			do_clocal = 1;
	} else {
		if (tty->termios->c_cflag & CLOCAL)
			do_clocal = 1;
	}
	
	/*
	 * Block waiting for the carrier detect and the line to become
	 * free (i.e., not in use by the callout).  While we are in
	 * this loop, state->count is dropped by one, so that
	 * rs_close() knows when to free things.  We restore it upon
	 * exit, either normal or abnormal.
	 */
	retval = 0;
	add_wait_queue(&info->open_wait, &wait);
#ifdef SERIAL_DEBUG_OPEN
	printk("block_til_ready before block: ttys%d, count = %d\n",
	       state->line, state->count);
#endif
	save_flags(flags); cli();
	if (!tty_hung_up_p(filp)) {
		extra_count = 1;
		state->count--;
	}
	restore_flags(flags);
	info->blocked_open++;
	while (1) {
		save_flags(flags); cli();
		if (!(info->flags & ASYNC_CALLOUT_ACTIVE) &&
		    (tty->termios->c_cflag & CBAUD))
			serial_out(info, SERIAL_MCR,
				   serial_inp(info, SERIAL_MCR) |
				   (SERIAL_MCR_DTR | SERIAL_MCR_RTS));
		restore_flags(flags);
		set_current_state(TASK_INTERRUPTIBLE);
		if (tty_hung_up_p(filp) ||
		    !(info->flags & ASYNC_INITIALIZED)) {
#ifdef SERIAL_DO_RESTART
			if (info->flags & ASYNC_HUP_NOTIFY)
				retval = -EAGAIN;
			else
				retval = -ERESTARTSYS;	
#else
			retval = -EAGAIN;
#endif
			break;
		}
		if (!(info->flags & ASYNC_CALLOUT_ACTIVE) &&
		    !(info->flags & ASYNC_CLOSING) &&
		    (do_clocal || (serial_in(info, SERIAL_MSR) &
				   SERIAL_MSR_DCD)))
			break;
		if (signal_pending(current)) {
			retval = -ERESTARTSYS;
			break;
		}
#ifdef SERIAL_DEBUG_OPEN
		printk("block_til_ready blocking: ttys%d, count = %d\n",
		       info->line, state->count);
#endif
		schedule();
	}
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&info->open_wait, &wait);
	if (extra_count)
		state->count++;
	info->blocked_open--;
#ifdef SERIAL_DEBUG_OPEN
	printk("block_til_ready after blocking: ttys%d, count = %d\n",
	       info->line, state->count);
#endif
	if (retval)
		return retval;
	info->flags |= ASYNC_NORMAL_ACTIVE;
	return 0;
}

static int get_async_struct(int line, struct async_struct **ret_info)
{
	struct async_struct *info;
	struct serial_state *sstate;

	sstate = rs_table + line;
	sstate->count++;
	if (sstate->info) {
		*ret_info = sstate->info;
		return 0;
	}
	info = kmalloc(sizeof(struct async_struct), GFP_KERNEL);
	if (!info) {
		sstate->count--;
		return -ENOMEM;
	}
	memset(info, 0, sizeof(struct async_struct));
	init_waitqueue_head(&info->open_wait);
	init_waitqueue_head(&info->close_wait);
	init_waitqueue_head(&info->delta_msr_wait);
	info->magic = SERIAL_MAGIC;
	info->port = sstate->port;
	info->flags = sstate->flags;
	info->io_type = sstate->io_type;
	info->iomem_base = sstate->iomem_base;
	info->iomem_reg_shift = sstate->iomem_reg_shift;
	info->xmit_fifo_size = sstate->xmit_fifo_size;
	info->line = line;
	info->tqueue.routine = do_softint;
	info->tqueue.data = info;
	info->state = sstate;
	if (sstate->info) {
		kfree(info);
		*ret_info = sstate->info;
		return 0;
	}
	*ret_info = sstate->info = info;
	return 0;
}

/*
 * This routine is called whenever a serial port is opened.  It
 * enables interrupts for a serial port, linking in its async structure into
 * the IRQ chain.   It also performs the serial-specific
 * initialization for the tty structure.
 *
 * Note that on failure, we don't decrement the module use count - the tty
 * later will call rs_close, which will decrement it for us as long as
 * tty->driver_data is set non-NULL. --rmk
 */
static int rs_open(struct tty_struct *tty, struct file * filp)
{
	struct async_struct	*info;
	int 			retval, line;
	unsigned long		page;

	MOD_INC_USE_COUNT;
	line = MINOR(tty->device) - tty->driver.minor_start;
	if ((line < 0) || (line >= NR_PORTS)) {
		MOD_DEC_USE_COUNT;
		return -ENODEV;
	}
	retval = get_async_struct(line, &info);
	if (retval) {
		MOD_DEC_USE_COUNT;
		return retval;
	}
	tty->driver_data = info;
	info->tty = tty;
	if (serial_paranoia_check(info, tty->device, "rs_open"))
		return -ENODEV;

#ifdef SERIAL_DEBUG_OPEN
	printk("rs_open %s%d, count = %d\n", tty->driver.name, info->line,
	       info->state->count);
#endif
#if (LINUX_VERSION_CODE > 0x20100)
	info->tty->low_latency = (info->flags & ASYNC_LOW_LATENCY) ? 1 : 0;
#endif

	/*
	 *	This relies on lock_kernel() stuff so wants tidying for 2.5
	 */
	if (!tmp_buf) {
		page = get_zeroed_page(GFP_KERNEL);
		if (!page)
			return -ENOMEM;
		if (tmp_buf)
			free_page(page);
		else
			tmp_buf = (unsigned char *) page;
	}

	/*
	 * If the port is the middle of closing, bail out now
	 */
	if (tty_hung_up_p(filp) ||
	    (info->flags & ASYNC_CLOSING)) {
		if (info->flags & ASYNC_CLOSING)
			interruptible_sleep_on(&info->close_wait);
#ifdef SERIAL_DO_RESTART
		return ((info->flags & ASYNC_HUP_NOTIFY) ?
			-EAGAIN : -ERESTARTSYS);
#else
		return -EAGAIN;
#endif
	}

	/*
	 * Start up serial port
	 */
	retval = startup(info);
	if (retval)
		return retval;

	retval = block_til_ready(tty, filp, info);
	if (retval) {
#ifdef SERIAL_DEBUG_OPEN
		printk("rs_open returning after block_til_ready with %d\n",
		       retval);
#endif
		return retval;
	}

	if ((info->state->count == 1) &&
	    (info->flags & ASYNC_SPLIT_TERMIOS)) {
		if (tty->driver.subtype == SERIAL_TYPE_NORMAL)
			*tty->termios = info->state->normal_termios;
		else 
			*tty->termios = info->state->callout_termios;
		change_speed(info, 0);
	}
#ifdef CONFIG_SERIAL_CPE_CONSOLE
	if (sercons.cflag && sercons.index == line) {
		tty->termios->c_cflag = sercons.cflag;
		sercons.cflag = 0;
		change_speed(info, 0);
	}
#endif
	info->session = current->session;
	info->pgrp = current->pgrp;

#ifdef SERIAL_DEBUG_OPEN
	printk("rs_open ttys%d successful...", info->line);
#endif
	return 0;
}

/*
 * /proc fs routines....
 */

static inline int line_info(char *buf, struct serial_state *state)
{
	struct async_struct *info = state->info, scr_info;
	char	stat_buf[30], control, status;
	int	ret;
	unsigned long flags;

	ret = sprintf(buf, "%d: uart:%s port:%lX irq:%d fifo:%d",
		      state->line, uart_config[state->type].name, 
		      state->port, state->irq, state->xmit_fifo_size);

	if (!state->port || (state->type == PORT_UNKNOWN)) {
		ret += sprintf(buf+ret, "\n");
		return ret;
	}

	/*
	 * Figure out the current RS-232 lines
	 */
	if (!info) {
		info = &scr_info;	/* This is just for serial_{in,out} */

		info->magic = SERIAL_MAGIC;
		info->port = state->port;
		info->flags = state->flags;
		info->hub6 = state->hub6;
		info->io_type = state->io_type;
		info->iomem_base = state->iomem_base;
		info->iomem_reg_shift = state->iomem_reg_shift;
		info->quot = 0;
		info->tty = 0;
	}
	save_flags(flags); cli();
	status = serial_in(info, SERIAL_MSR);
	control = info != &scr_info ? info->MCR : serial_in(info, SERIAL_MCR);
	restore_flags(flags); 

	stat_buf[0] = 0;
	stat_buf[1] = 0;
	if (control & SERIAL_MCR_RTS)
		strcat(stat_buf, "|RTS");
	if (status & SERIAL_MSR_CTS)
		strcat(stat_buf, "|CTS");
	if (control & SERIAL_MCR_DTR)
		strcat(stat_buf, "|DTR");
	if (status & SERIAL_MSR_DSR)
		strcat(stat_buf, "|DSR");
	if (status & SERIAL_MSR_DCD)
		strcat(stat_buf, "|CD");
	if (status & SERIAL_MSR_RI)
		strcat(stat_buf, "|RI");

	if (info->quot) {
		ret += sprintf(buf+ret, " baud:%d",
			       state->baud_base / info->quot);
	}

	ret += sprintf(buf+ret, " tx:%d rx:%d",
		      state->icount.tx, state->icount.rx);

	if (state->icount.frame)
		ret += sprintf(buf+ret, " fe:%d", state->icount.frame);
	
	if (state->icount.parity)
		ret += sprintf(buf+ret, " pe:%d", state->icount.parity);
	
	if (state->icount.brk)
		ret += sprintf(buf+ret, " brk:%d", state->icount.brk);	

	if (state->icount.overrun)
		ret += sprintf(buf+ret, " oe:%d", state->icount.overrun);

	/*
	 * Last thing is the RS-232 status lines
	 */
	ret += sprintf(buf+ret, " %s\n", stat_buf+1);
	return ret;
}


int rs_read_proc(char *page, char **start, off_t off, int count,
		 int *eof, void *data)
{
	int i, len = 0, l;
	off_t	begin = 0;

	len += sprintf(page, "serinfo:1.0 driver:%s%s revision:%s\n",
		       serial_version, LOCAL_VERSTRING, serial_revdate);
	for (i = 0; i < NR_PORTS && len < 4000; i++) {
		l = line_info(page + len, &rs_table[i]);
		len += l;
		if (len+begin > off+count)
			goto done;
		if (len+begin < off) {
			begin += len;
			len = 0;
		}
	}
	*eof = 1;
done:
	if (off >= len+begin)
		return 0;
	*start = page + (off-begin);
	return ((count < begin+len-off) ? count : begin+len-off);
}

/*
 * ---------------------------------------------------------------------
 * rs_init() and friends
 *
 * rs_init() is called at boot-time to initialize the serial driver.
 * ---------------------------------------------------------------------
 */

/*
 * This routine prints out the appropriate serial driver version
 * number, and identifies which options were configured into this
 * driver.
 */
static char serial_options[] __initdata =

#ifdef CONFIG_SERIAL_SHARE_IRQ
       " SHARE_IRQ"
#define SERIAL_OPT
#endif
#ifdef CONFIG_SERIAL_DETECT_IRQ
       " DETECT_IRQ"
#define SERIAL_OPT
#endif
#ifdef ENABLE_SERIAL_ACPI
       " SERIAL_ACPI"
#define SERIAL_OPT
#endif
#ifdef SERIAL_OPT
       " enabled\n";
#else
       " no serial options enabled\n";
#endif
#undef SERIAL_OPT

static _INLINE_ void show_serial_version(void)
{
 	printk(KERN_INFO "%s version %s%s (%s) with%s", serial_name,
	       serial_version, LOCAL_VERSTRING, serial_revdate,
	       serial_options);
}

/*
 * This routine detect the IRQ of a serial port by clearing OUT2 when
 * no UART interrupt are requested (IER = 0) (*GPL*). This seems to work at
 * each time, as long as no other device permanently request the IRQ.
 * If no IRQ is detected, or multiple IRQ appear, this function returns 0.
 * The variable "state" and the field "state->port" should not be null.
 */
static unsigned detect_uart_irq (struct serial_state * state)
{
	int irq;
	unsigned long irqs;
	unsigned char save_mcr, save_ier;
	struct async_struct scr_info; /* serial_{in,out} because HUB6 */

	scr_info.magic = SERIAL_MAGIC;
	scr_info.state = state;
	scr_info.port = state->port;
	scr_info.flags = state->flags;

	scr_info.io_type = state->io_type;
	scr_info.iomem_base = state->iomem_base;
	scr_info.iomem_reg_shift = state->iomem_reg_shift;

	/* forget possible initially masked and pending IRQ */
	probe_irq_off(probe_irq_on());
	save_mcr = serial_inp(&scr_info, SERIAL_MCR);
	save_ier = serial_inp(&scr_info, SERIAL_IER);
	serial_outp(&scr_info, SERIAL_MCR, SERIAL_MCR_OUT1 | SERIAL_MCR_OUT2);
	
	irqs = probe_irq_on();
	serial_outp(&scr_info, SERIAL_MCR, 0);
	udelay (10);
	if (state->flags & ASYNC_FOURPORT)  {
		serial_outp(&scr_info, SERIAL_MCR,
			    SERIAL_MCR_DTR | SERIAL_MCR_RTS);
	} else {
		serial_outp(&scr_info, SERIAL_MCR,
			    SERIAL_MCR_DTR | SERIAL_MCR_RTS | SERIAL_MCR_OUT2);
	}
	serial_outp(&scr_info, SERIAL_IER, 0x0f);	/* enable all intrs */
	(void)serial_inp(&scr_info, SERIAL_LSR);
	(void)serial_inp(&scr_info, SERIAL_RBR);
	(void)serial_inp(&scr_info, SERIAL_IIR);
	(void)serial_inp(&scr_info, SERIAL_MSR);
	serial_outp(&scr_info, SERIAL_THR, 0xFF);
	udelay (20);
	irq = probe_irq_off(irqs);

	serial_outp(&scr_info, SERIAL_MCR, save_mcr);
	serial_outp(&scr_info, SERIAL_IER, save_ier);

	return (irq > 0)? irq : 0;
}

#ifdef not_complete_yet
/*
 * This is a quickie test to see how big the FIFO is.
 * It doesn't work at all the time, more's the pity.
 */
static int size_fifo(struct async_struct *info)
{
	unsigned char old_fcr, old_mcr, old_dll, old_dlm;
	int count;

	old_fcr = serial_inp(info, SERIAL_FCR);
	old_mcr = serial_inp(info, SERIAL_MCR);
	serial_outp(info, SERIAL_FCR, SERIAL_FCR_FE |
		    SERIAL_FCR_RXFR | SERIAL_FCR_TXFR);
	serial_outp(info, SERIAL_MCR, SERIAL_MCR_LPBK);
	serial_outp(info, SERIAL_LCR, SERIAL_LCR_DLAB);
	old_dll = serial_inp(info, SERIAL_DLL);
	old_dlm = serial_inp(info, SERIAL_DLM);
	serial_outp(info, SERIAL_DLL, 0x01);
	serial_outp(info, SERIAL_DLM, 0x00);
	serial_outp(info, SERIAL_LCR, 0x03);
	for (count = 0; count < 256; count++)
		serial_outp(info, SERIAL_THR, count);
	mdelay(20);
	for (count = 0; (serial_inp(info, SERIAL_LSR) & SERIAL_LSR_DR) &&
	     (count < 256); count++)
		serial_inp(info, SERIAL_RBR);
	serial_outp(info, SERIAL_FCR, old_fcr);
	serial_outp(info, SERIAL_MCR, old_mcr);
	serial_outp(info, SERIAL_LCR, SERIAL_LCR_DLAB);
	serial_outp(info, SERIAL_DLL, old_dll);
	serial_outp(info, SERIAL_DLM, old_dlm);

	return count;
}
#endif /* end_of_not */



/*
 * This routine is called by rs_init() to initialize a specific serial
 * port.  It determines what type of UART chip this serial port is
 * using: 8250, 16450, 16550, 16550A.  The important question is
 * whether or not this UART is a 16550A or not, since this will
 * determine whether or not we can use its FIFO features or not.
 */
static void autoconfig(struct serial_state * state)
{
	unsigned char status1, status2, scratch, scratch2, scratch3;
	unsigned char save_lcr, save_mcr;
	struct async_struct *info, scr_info;
	unsigned long flags;

	state->type = PORT_UNKNOWN;

#ifdef SERIAL_DEBUG_AUTOCONF
	printk("Testing ttyS%d (0x%04lx, 0x%04x)...\n", state->line,
	       state->port, (unsigned) state->iomem_base);
#endif
	
	if (!CONFIGURED_SERIAL_PORT(state))
		return;
		
	info = &scr_info;	/* This is just for serial_{in,out} */

	info->magic = SERIAL_MAGIC;
	info->state = state;
	info->port = state->port;
	info->flags = state->flags;

	info->io_type = state->io_type;
	info->iomem_base = state->iomem_base;
	info->iomem_reg_shift = state->iomem_reg_shift;

	save_flags(flags); cli();
	
	if (!(state->flags & ASYNC_BUGGY_UART) &&
	    !state->iomem_base) {
		/*
		 * Do a simple existence test first; if we fail this,
		 * there's no point trying anything else.
		 * 
		 * 0x80 is used as a nonsense port to prevent against
		 * false positives due to ISA bus float.  The
		 * assumption is that 0x80 is a non-existent port;
		 * which should be safe since include/asm/io.h also
		 * makes this assumption.
		 */
		scratch = serial_inp(info, SERIAL_IER);
		serial_outp(info, SERIAL_IER, 0);

		scratch2 = serial_inp(info, SERIAL_IER);
		serial_outp(info, SERIAL_IER, 0x0F);

		scratch3 = serial_inp(info, SERIAL_IER);
		serial_outp(info, SERIAL_IER, scratch);
		if (scratch2 || scratch3 != 0x0F) {
#ifdef SERIAL_DEBUG_AUTOCONF
			printk("serial: ttyS%d: simple autoconfig failed "
			       "(%02x, %02x)\n", state->line, 
			       scratch2, scratch3);
#endif
			restore_flags(flags);
			return;		/* We failed; there's nothing here */
		}
	}

	save_mcr = serial_in(info, SERIAL_MCR);
	save_lcr = serial_in(info, SERIAL_LCR);

	/* 
	 * Check to see if a UART is really there.  Certain broken
	 * internal modems based on the Rockwell chipset fail this
	 * test, because they apparently don't implement the loopback
	 * test mode.  So this test is skipped on the COM 1 through
	 * COM 4 ports.  This *should* be safe, since no board
	 * manufacturer would be stupid enough to design a board
	 * that conflicts with COM 1-4 --- we hope!
	 */
	if (!(state->flags & ASYNC_SKIP_TEST)) {
		serial_outp(info, SERIAL_MCR, SERIAL_MCR_LPBK | 0x0A);
		status1 = serial_inp(info, SERIAL_MSR) & 0xF0;
		serial_outp(info, SERIAL_MCR, save_mcr);
		if (status1 != 0x90) {
#ifdef SERIAL_DEBUG_AUTOCONF
			printk("serial: ttyS%d: no UART loopback failed\n",
			       state->line);
#endif
			restore_flags(flags);
			return;
		}
	}
	serial_outp(info, SERIAL_LCR, 0xBF); /* set up for StarTech test */
	serial_outp(info, SERIAL_EFR, 0);	/* EFR is the same as FCR */
	serial_outp(info, SERIAL_LCR, 0);
	serial_outp(info, SERIAL_FCR, SERIAL_FCR_FE);
	scratch = serial_in(info, SERIAL_IIR) >> 6;
	switch (scratch) {
		case 0:
			state->type = PORT_16450;
			break;
		case 1:
			state->type = PORT_UNKNOWN;
			break;
		case 2:
			state->type = PORT_16550;
			break;
		case 3:
			state->type = PORT_16550A;
			break;
	}



	serial_outp(info, SERIAL_LCR, save_lcr);
	if (state->type == PORT_16450) {
		scratch = serial_in(info, SERIAL_SPR);
		serial_outp(info, SERIAL_SPR, 0xa5);
		status1 = serial_in(info, SERIAL_SPR);
		serial_outp(info, SERIAL_SPR, 0x5a);
		status2 = serial_in(info, SERIAL_SPR);
		serial_outp(info, SERIAL_SPR, scratch);

		if ((status1 != 0xa5) || (status2 != 0x5a))
			state->type = PORT_8250;
	}
	state->xmit_fifo_size =	uart_config[state->type].dfl_xmit_fifo_size;
//ivan
	printk("state->type=%s fifo size=%d\r\n",uart_config[state->type].name,state->xmit_fifo_size);

	if (state->type == PORT_UNKNOWN) {
		restore_flags(flags);
		return;
	}

	if (info->port) {
			//request_region(info->port,8,"serial(auto)");
			request_region(info->port, 0x40, "serial(auto)");
	}

	/*
	 * Reset the UART.
	 */
	serial_outp(info, SERIAL_MCR, save_mcr);
	serial_outp(info, SERIAL_FCR, (SERIAL_FCR_FE |
				     SERIAL_FCR_RXFR |
				     SERIAL_FCR_TXFR));
	serial_outp(info, SERIAL_FCR, 0);
	(void)serial_in(info, SERIAL_RBR);
	serial_outp(info, SERIAL_IER, 0);
	
	restore_flags(flags);
}


int register_serial(struct serial_struct *req);
void unregister_serial(int line);

#if (LINUX_VERSION_CODE > 0x20100)
EXPORT_SYMBOL(register_serial);
EXPORT_SYMBOL(unregister_serial);
#else
static struct symbol_table serial_syms = {
#include <linux/symtab_begin.h>
	X(register_serial),
	X(unregister_serial),
#include <linux/symtab_end.h>
};
#endif






/*
 * The serial driver boot-time initialization code!
 */
static int __init rs_init(void)
{
	int i;
	struct serial_state * state;
//printk("rs_init\r\n");
	init_bh(SERIAL_BH, do_serial_bh);
	init_timer(&serial_timer);
	serial_timer.function = rs_timer;
	mod_timer(&serial_timer, jiffies + RS_STROBE_TIME);

	for (i = 0; i < NR_IRQS; i++) {
		IRQ_ports[i] = 0;
		IRQ_timeout[i] = 0;
	}
#ifdef CONFIG_SERIAL_CPE_CONSOLE
	/*
	 *	The interrupt of the serial console port
	 *	can't be shared.
	 */
	if (sercons.flags & CON_CONSDEV) {
		for(i = 0; i < NR_PORTS; i++)
			if (i != sercons.index &&
			    rs_table[i].irq == rs_table[sercons.index].irq)
				rs_table[i].irq = 0;
	}
#endif
	show_serial_version();

	/* Initialize the tty_driver structure */
	
	memset(&serial_driver, 0, sizeof(struct tty_driver));
	serial_driver.magic = TTY_DRIVER_MAGIC;
#if (LINUX_VERSION_CODE > 0x20100)
	serial_driver.driver_name = "serial";
#endif
#if (LINUX_VERSION_CODE > 0x2032D && defined(CONFIG_DEVFS_FS))
	serial_driver.name = "tts/%d";
#else
	serial_driver.name = "ttyS";
#endif
	serial_driver.major = TTY_MAJOR;
	serial_driver.minor_start = 64 + SERIAL_DEV_OFFSET;
	serial_driver.num = NR_PORTS;
	serial_driver.type = TTY_DRIVER_TYPE_SERIAL;
	serial_driver.subtype = SERIAL_TYPE_NORMAL;
	serial_driver.init_termios = tty_std_termios;
	serial_driver.init_termios.c_cflag =
		B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	serial_driver.flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_NO_DEVFS;
	serial_driver.refcount = &serial_refcount;
	serial_driver.table = serial_table;
	serial_driver.termios = serial_termios;
	serial_driver.termios_locked = serial_termios_locked;

	serial_driver.open = rs_open;
	serial_driver.close = rs_close;
	serial_driver.write = rs_write;
	serial_driver.put_char = rs_put_char;
	serial_driver.flush_chars = rs_flush_chars;
	serial_driver.write_room = rs_write_room;
	serial_driver.chars_in_buffer = rs_chars_in_buffer;
	serial_driver.flush_buffer = rs_flush_buffer;
	serial_driver.ioctl = rs_ioctl;
	serial_driver.throttle = rs_throttle;
	serial_driver.unthrottle = rs_unthrottle;
	serial_driver.set_termios = rs_set_termios;
	serial_driver.stop = rs_stop;
	serial_driver.start = rs_start;
	serial_driver.hangup = rs_hangup;
#if (LINUX_VERSION_CODE >= 131394) /* Linux 2.1.66 */
	serial_driver.break_ctl = rs_break;
#endif
#if (LINUX_VERSION_CODE >= 131343)
	serial_driver.send_xchar = rs_send_xchar;
	serial_driver.wait_until_sent = rs_wait_until_sent;
	serial_driver.read_proc = rs_read_proc;
#endif
	
	/*
	 * The callout device is just like normal device except for
	 * major number and the subtype code.
	 */
	callout_driver = serial_driver;
#if (LINUX_VERSION_CODE > 0x2032D && defined(CONFIG_DEVFS_FS))
	callout_driver.name = "cua/%d";
#else
	callout_driver.name = "cua";
#endif
	callout_driver.major = TTYAUX_MAJOR;
	callout_driver.subtype = SERIAL_TYPE_CALLOUT;
#if (LINUX_VERSION_CODE >= 131343)
	callout_driver.read_proc = 0;
	callout_driver.proc_entry = 0;
#endif

	if (tty_register_driver(&serial_driver))
		panic("Couldn't register serial driver\n");
	if (tty_register_driver(&callout_driver))
		panic("Couldn't register callout driver\n");
	

	// ªì©l¤Æ serial_state
	for (i = 0, state = rs_table; i < NR_PORTS; i++,state++) 
	{
		state->magic = SSTATE_MAGIC;
		state->line = i;
		state->type = PORT_FARADAY;				//state->type = PORT_UNKNOWN;
		state->custom_divisor = 0;
		state->close_delay = 5*HZ/10;
		state->closing_wait = 30*HZ;
		state->callout_termios = callout_driver.init_termios;
		state->normal_termios = serial_driver.init_termios;
		state->icount.cts = state->icount.dsr = 
			state->icount.rng = state->icount.dcd = 0;
		state->icount.rx = state->icount.tx = 0;
		state->icount.frame = state->icount.parity = 0;
		state->icount.overrun = state->icount.brk = 0;
		state->irq = irq_cannonicalize(state->irq);
		if (state->hub6)
			state->io_type = SERIAL_IO_HUB6;
		if (state->port && check_region(state->port, 0x40))
			continue;

		if (state->flags & ASYNC_BOOT_AUTOCONF)
			autoconfig(state);
	}

	// µù¥U devfs
	for (i = 0, state = rs_table; i < NR_PORTS; i++,state++) 
	{
		if (state->type == PORT_UNKNOWN)
			continue;
		if (   (state->flags & ASYNC_BOOT_AUTOCONF)
		    && (state->flags & ASYNC_AUTO_IRQ)
		    && (state->port != 0 || state->iomem_base != 0))
			state->irq = detect_uart_irq(state);
		if (state->io_type == SERIAL_IO_MEM) {
			printk(KERN_INFO"ttyS%02d%s at 0x%px (irq = %d) is a %s\n",
			//printk(KERN_INFO"ttyAM%d%s at 0x%px (irq = %d) is a %s\n",
	 		       state->line + SERIAL_DEV_OFFSET,
			       (state->flags & ASYNC_FOURPORT) ? " FourPort" : "",
			       state->iomem_base, state->irq,
			       uart_config[state->type].name);
		}
		else {
			printk(KERN_INFO "ttyS%02d%s at 0x%04lx (irq = %d) is a %s\n",
//			printk(KERN_INFO "ttyAM%d%s at 0x%04lx (irq = %d) is a %s\n",
	 		       state->line + SERIAL_DEV_OFFSET,
			       (state->flags & ASYNC_FOURPORT) ? " FourPort" : "",
			       state->port, state->irq,
			       uart_config[state->type].name);
		}
		tty_register_devfs(&serial_driver, 0,
				   serial_driver.minor_start + state->line);
		tty_register_devfs(&callout_driver, 0,
				   callout_driver.minor_start + state->line);
	}
	return 0;
}

#ifdef not_complete_yet
/*
 * This is for use by architectures that know their serial console 
 * attributes only at run time. Not to be invoked after rs_init().
 */
int __init early_serial_setup(struct serial_struct *req)
{
	int i = req->line;

	if (i >= NR_IRQS)
		return(-ENOENT);
	rs_table[i].magic = 0;
	rs_table[i].baud_base = req->baud_base;
	rs_table[i].port = req->port;
	if (HIGH_BITS_OFFSET)
		rs_table[i].port += (unsigned long) req->port_high << 
							HIGH_BITS_OFFSET;
	rs_table[i].irq = req->irq;
	rs_table[i].flags = req->flags;
	rs_table[i].close_delay = req->close_delay;
	rs_table[i].io_type = req->io_type;
	rs_table[i].hub6 = req->hub6;
	rs_table[i].iomem_base = req->iomem_base;
	rs_table[i].iomem_reg_shift = req->iomem_reg_shift;
	rs_table[i].type = req->type;
	rs_table[i].xmit_fifo_size = req->xmit_fifo_size;
	rs_table[i].custom_divisor = req->custom_divisor;
	rs_table[i].closing_wait = req->closing_wait;
	return(0);
}
#endif /* end_of_not */


/*
 * register_serial and unregister_serial allows for 16x50 serial ports to be
 * configured at run-time, to support PCMCIA modems.
 */
 
/**
 *	register_serial - configure a 16x50 serial port at runtime
 *	@req: request structure
 *
 *	Configure the serial port specified by the request. If the
 *	port exists and is in use an error is returned. If the port
 *	is not currently in the table it is added.
 *
 *	The port is then probed and if neccessary the IRQ is autodetected
 *	If this fails an error is returned.
 *
 *	On success the port is ready to use and the line number is returned.
 */
 
int register_serial(struct serial_struct *req)
{
	int i;
	unsigned long flags;
	struct serial_state *state;
	struct async_struct *info;
	unsigned long port;

	port = req->port;
	if (HIGH_BITS_OFFSET)
		port += (unsigned long) req->port_high << HIGH_BITS_OFFSET;

	save_flags(flags); cli();
	for (i = 0; i < NR_PORTS; i++) {
		if ((rs_table[i].port == port) &&
		    (rs_table[i].iomem_base == req->iomem_base))
			break;
	}

	if (i == NR_PORTS) {
		for (i = 0; i < NR_PORTS; i++)
			if ((rs_table[i].type == PORT_UNKNOWN) &&
			    (rs_table[i].count == 0))
				break;
	}
	if (i == NR_PORTS) {
		restore_flags(flags);
		return -1;
	}
	state = &rs_table[i];
	if (rs_table[i].count) {
		restore_flags(flags);
		printk("Couldn't configure serial #%d (port=%ld,irq=%d): "
		       "device already open\n", i, port, req->irq);
		return -1;
	}
	state->irq = req->irq;
	state->port = port;
	state->flags = req->flags;
	state->io_type = req->io_type;
	state->iomem_base = req->iomem_base;
	state->iomem_reg_shift = req->iomem_reg_shift;
	if (req->baud_base)
		state->baud_base = req->baud_base;
	if ((info = state->info) != NULL) {
		info->port = port;
		info->flags = req->flags;
		info->io_type = req->io_type;
		info->iomem_base = req->iomem_base;
		info->iomem_reg_shift = req->iomem_reg_shift;
	}
	autoconfig(state);
	if (state->type == PORT_UNKNOWN) {
		restore_flags(flags);
		printk("register_serial(): autoconfig failed\n");
		return -1;
	}
	restore_flags(flags);

	if ((state->flags & ASYNC_AUTO_IRQ) && CONFIGURED_SERIAL_PORT(state))
		state->irq = detect_uart_irq(state);

//       printk(KERN_INFO "ttyAM%d at %s 0x%04lx (irq = %d) is a %s\n",
       printk(KERN_INFO "ttyS%02d at %s 0x%04lx (irq = %d) is a %s\n",
	      state->line + SERIAL_DEV_OFFSET,
	      state->iomem_base ? "iomem" : "port",
	      state->iomem_base ? (unsigned long)state->iomem_base :
	      state->port, state->irq, uart_config[state->type].name);
	tty_register_devfs(&serial_driver, 0,
			   serial_driver.minor_start + state->line); 
	tty_register_devfs(&callout_driver, 0,
			   callout_driver.minor_start + state->line);
	return state->line + SERIAL_DEV_OFFSET;
}

/**
 *	unregister_serial - deconfigure a 16x50 serial port
 *	@line: line to deconfigure
 *
 *	The port specified is deconfigured and its resources are freed. Any
 *	user of the port is disconnected as if carrier was dropped. Line is
 *	the port number returned by register_serial().
 */

void unregister_serial(int line)
{
	unsigned long flags;
	struct serial_state *state = &rs_table[line];

	save_flags(flags); cli();
	if (state->info && state->info->tty)
		tty_hangup(state->info->tty);
	state->type = PORT_UNKNOWN;
	printk(KERN_INFO "tty%02d unloaded\n", state->line);
	/* These will be hidden, because they are devices that will no longer
	 * be available to the system. (ie, PCMCIA modems, once ejected)
	 */
	tty_unregister_devfs(&serial_driver,
			     serial_driver.minor_start + state->line);
	tty_unregister_devfs(&callout_driver,
			     callout_driver.minor_start + state->line);
	restore_flags(flags);
}

static void __exit rs_fini(void) 
{
	unsigned long flags;
	int e1, e2;
	int i;
	struct async_struct *info;

	/* printk("Unloading %s: version %s\n", serial_name, serial_version); */
	del_timer_sync(&serial_timer);
	save_flags(flags); cli();
        remove_bh(SERIAL_BH);
	if ((e1 = tty_unregister_driver(&serial_driver)))
		printk("serial: failed to unregister serial driver (%d)\n",
		       e1);
	if ((e2 = tty_unregister_driver(&callout_driver)))
		printk("serial: failed to unregister callout driver (%d)\n", 
		       e2);
	restore_flags(flags);

	for (i = 0; i < NR_PORTS; i++) {
		if ((info = rs_table[i].info)) {
			rs_table[i].info = NULL;
			kfree(info);
		}
		if ((rs_table[i].type != PORT_UNKNOWN) && rs_table[i].port) {
				release_region(rs_table[i].port, 8);
		}

	}

	if (tmp_buf) {
		unsigned long pg = (unsigned long) tmp_buf;
		tmp_buf = NULL;
		free_page(pg);
	}
	
}

module_init(rs_init);
module_exit(rs_fini);
MODULE_DESCRIPTION("Standard/generic (dumb) serial driver");
MODULE_AUTHOR("Theodore Ts'o <tytso@mit.edu>");
MODULE_LICENSE("GPL");


/*
 * ------------------------------------------------------------
 * Serial console driver
 * ------------------------------------------------------------
 */
#ifdef CONFIG_SERIAL_CPE_CONSOLE

#define BOTH_EMPTY (SERIAL_LSR_TE | SERIAL_LSR_THRE)

static struct async_struct async_sercons;

/*
 *	Wait for transmitter & holding register to empty
 */
static inline void wait_for_xmitr(struct async_struct *info)
{
	unsigned int status, tmout = 1000000;

	do {
		status = serial_in(info, SERIAL_LSR);

		if (status & SERIAL_LSR_BI)
			lsr_break_flag = SERIAL_LSR_BI;
		
		if (--tmout == 0)
			break;
	} while((status & BOTH_EMPTY) != BOTH_EMPTY);

	/* Wait for flow control if necessary */
	if (info->flags & ASYNC_CONS_FLOW) {
		tmout = 1000000;
		while (--tmout &&
		       ((serial_in(info, SERIAL_MSR) & SERIAL_MSR_CTS) == 0));
	}	
}


/*
 *	Print a string to the serial port trying not to disturb
 *	any possible real use of the port...
 *
 *	The console must be locked when we get here.
 */
static void serial_console_write(struct console *co, const char *s,
				unsigned count)
{
	static struct async_struct *info = &async_sercons;
	int ier;
	unsigned i;

	/*
	 *	First save the IER then disable the interrupts
	 */
	ier = serial_in(info, SERIAL_IER);
	serial_out(info, SERIAL_IER, 0x00);

	/*
	 *	Now, do each character
	 */
	for (i = 0; i < count; i++, s++) {
		wait_for_xmitr(info);

		/*
		 *	Send the character out.
		 *	If a LF, also do CR...
		 */
		serial_out(info, SERIAL_THR, *s);
		if (*s == 10) {
			wait_for_xmitr(info);
			serial_out(info, SERIAL_THR, 13);
		}
	}

	/*
	 *	Finally, Wait for transmitter & holding register to empty
	 * 	and restore the IER
	 */
	wait_for_xmitr(info);
	serial_out(info, SERIAL_IER, ier);
}

#ifdef not_complete_yet
/*
 *	Receive character from the serial port
 */
static int serial_console_wait_key(struct console *co)
{
	static struct async_struct *info;
	int ier, c;

	info = &async_sercons;

	/*
	 *	First save the IER then disable the interrupts so
	 *	that the real driver for the port does not get the
	 *	character.
	 */
	ier = serial_in(info, SERIAL_IER);
	serial_out(info, SERIAL_IER, 0x00);
 
	while ((serial_in(info, SERIAL_LSR) & SERIAL_LSR_DR) == 0);
	c = serial_in(info, SERIAL_RBR);

	/*
	 *	Restore the interrupts
	 */
	serial_out(info, SERIAL_IER, ier);

	return c;
}
#endif /* end_of_not */


static kdev_t serial_console_device(struct console *c)
{
	return MKDEV(TTY_MAJOR, 64 + c->index);
}

/*
 *	Setup initial baud/bits/parity/flow control. We do two things here:
 *	- construct a cflag setting for the first rs_open()
 *	- initialize the serial port
 *	Return non-zero if we didn't find a serial port.
 */
static int __init serial_console_setup(struct console *co, char *options)
{
	static struct async_struct *info;
	struct serial_state *state;
	unsigned cval;
	int     baud = CONFIG_SERIAL_CPE_DEFAULT_BAUD;
	int	bits = 8;
	int	parity = 'n';
	int	doflow = 0;
	int	cflag = CREAD | HUPCL | CLOCAL;
	int	quot = 0;
	char	*s;

	
	if (options) 
	{
		baud = simple_strtoul(options, NULL, 10);
		s = options;
		while(*s >= '0' && *s <= '9')
			s++;
		if (*s) parity = *s++;
		if (*s) bits   = *s++ - '0';
		if (*s) doflow = (*s++ == 'r');
	}

	
	/*
	 *	Now construct a cflag setting.
	 */
	switch(baud) 
	{
		case 1200:
			cflag |= B1200;
			break;
		case 2400:
			cflag |= B2400;
			break;
		case 4800:
			cflag |= B4800;
			break;
		case 19200:
			cflag |= B19200;
			break;
		case 38400:
			cflag |= B38400;
			break;
		case 57600:
			cflag |= B57600;
			break;
		case 115200:
			cflag |= B115200;
			break;
		case 9600:
		default:
			cflag |= B9600;
			/*
			 * Set this to a sane value to prevent a divide error
			 */
			baud  = 9600;
			break;
	}
	switch(bits) {
		case 7:
			cflag |= CS7;
			break;
		default:
		case 8:
			cflag |= CS8;
			break;
	}
	switch(parity) {
		case 'o': case 'O':
			cflag |= PARODD;
			break;
		case 'e': case 'E':
			cflag |= PARENB;
			break;
	}
	co->cflag = cflag;

	/*
	 *	Divisor, bytesize and parity
	 */
	state = rs_table + co->index;
	if (doflow)
		state->flags |= ASYNC_CONS_FLOW;
	info = &async_sercons;
	info->magic = SERIAL_MAGIC;
	info->state = state;
	info->port = state->port;
	info->flags = state->flags;

	info->io_type = state->io_type;
	info->iomem_base = state->iomem_base;
	info->iomem_reg_shift = state->iomem_reg_shift;
	quot = state->baud_base / baud;
	cval = cflag & (CSIZE | CSTOPB);
	cval >>= 4;
	if (cflag & PARENB)
	{
		cval |= SERIAL_LCR_PE;
	}
	if (!(cflag & PARODD))
	{
		cval |= SERIAL_LCR_EVEN;
	}

	
	/*
	 *	Disable UART interrupts, set DTR and RTS high
	 *	and set speed.
	 */
	serial_out(info, SERIAL_LCR, cval | SERIAL_LCR_DLAB);	/* set DLAB */
	serial_out(info, SERIAL_DLL, quot & 0xff);	/* LS of divisor */
	serial_out(info, SERIAL_DLM, quot >> 8);		/* MS of divisor */
	serial_out(info, SERIAL_LCR, cval);		/* reset DLAB */
	serial_out(info, SERIAL_IER, 0);
	serial_out(info, SERIAL_MCR, SERIAL_MCR_DTR | SERIAL_MCR_RTS);

	/*
	 *	If we read 0xff from the LSR, there is no UART here.
	 */
	if (serial_in(info, SERIAL_LSR) == 0xff)
		return -1;

	return 0;
}

static struct console sercons = {
	name:		"ttyS",
	write:		serial_console_write,
	device:		serial_console_device,
///	wait_key:	serial_console_wait_key,
	setup:		serial_console_setup,
	flags:		CON_PRINTBUFFER,
	index:		-1,
};

/*
 *	Register console.
 */
void __init cpe_console_init(void)
{
	register_console(&sercons);
}
#endif

/*
  Local variables:
  compile-command: "gcc -D__KERNEL__ -I../../include -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -fno-strict-aliasing -pipe -fno-strength-reduce -march=i586 -DMODULE -DMODVERSIONS -include ../../include/linux/modversions.h   -DEXPORT_SYMTAB -c serial.c"
  End:
*/
