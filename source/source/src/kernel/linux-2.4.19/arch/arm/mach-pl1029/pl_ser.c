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
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
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
#include <linux/init.h>
#include <linux/circ_buf.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/serial_core.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/bitops.h>
#include <asm/hardware.h>

#ifdef CONFIG_MAGIC_SYSRQ
#define SUPPORT_SYSRQ
#endif

#define PORT_PLSER              40

#define SERIAL_PLSER_MAJOR      204
#define CALLOUT_PLSER_MAJOR     205
#define MINOR_START             5
#define NR_PORTS                1


static struct tty_driver normal, callout;
static struct tty_struct *plser_table[NR_PORTS];
static struct termios *plser_termios[NR_PORTS], *plser_termios_locked[NR_PORTS];

#ifdef SUPPORT_SYSRQ
static struct console plser_console;
#endif

/*
 * plser registers
 */
#define     SER_MEMBASE     PL_CONSOLE_BASE

#define     SER_STATUS      0x00
#define     SER_DATA        0x01
#define     SER_CON_RST     0x02
#define     SER_SRB_GEN     0x03
#define     SER_PERIOD      0x04
#define     SER_CONTROL     0x06
#define     SER_DETECT      0x07

#define     ST_RX_READY     0x80
#define     ST_OVERRUN      0x40
#define     ST_PARITY       0x20
#define     ST_FRAME        0x10
#define     ST_BREAK        0x08
#define     ST_TX_EMPTY     0x04
#define     ST_TX_FULL      0x02
#define     ST_CTS          0x01



/*
 * register accessing function
 */
#define     plser_readl(port, reg)          __raw_readl((port)->membase+(reg))
#define     plser_readw(port, reg)          __raw_readw((port)->membase+(reg))
#define     plser_readb(port, reg)          __raw_readb((port)->membase+(reg))
#define     plser_writel(port, v, reg)      __raw_writel((v), (port)->membase+(reg))
#define     plser_writew(port, v, reg)      __raw_writew((v), (port)->membase+(reg))
#define     plser_writeb(port, v, reg)      __raw_writeb((v), (port)->membase+(reg))

static int  plser_rx_chars(struct uart_info *info, struct pt_regs *regs);
static int  plser_tx_chars(struct uart_info *info);
static void plser_int(int irq, void *dev_id, struct pt_regs *regs);


/* avoid rx and tx status read clear issue */
unsigned long irq_latch = ~0;
unsigned long queue_latch = ~0;
struct tq_struct plser_task;    /* task to start transmit and receive by tq_timer or tq_immediate */

static void plser_task_action(void *data)
{
    struct uart_port *port = (struct uart_port *)data;
    struct uart_info *info;

    memcpy(&info, port->unused, sizeof(void *));
    plser_int(port->irq, info, NULL);
}

static void plser_delay_tx_task(struct uart_port *port)
{
    if (test_and_set_bit(port->irq, &queue_latch))
        return;

    queue_task(&plser_task, &tq_timer);
}

int ind = 0;
static void plser_stop_tx(struct uart_port *port, u_int from_tty)
{
	port->read_status_mask &= ~(ST_TX_FULL | ST_CTS);
}

static void plser_start_tx(struct uart_port *port, u_int nonempty, u_int from_tty)
{

    if (nonempty) {
        port->read_status_mask |= ST_TX_FULL | ST_CTS;
        queue_task(&plser_task, &tq_immediate);
        mark_bh(IMMEDIATE_BH);
    }
}

static void plser_stop_rx(struct uart_port *port)
{
    port->read_status_mask &= ~ST_RX_READY;
}


/*
 * No modem control lines
 */
static void plser_enable_ms(struct uart_port *port)
{
    /* do nothing */
}

static int
plser_rx_chars(struct uart_info *info, struct pt_regs *regs)
{
	struct tty_struct *tty = info->tty;
	unsigned int status, ch, flg, ignored = 0;
	struct uart_port *port = info->port;
    int rc = 0;

	status = plser_readb(port, SER_STATUS);
	while (status & ST_RX_READY) {
		if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
            tty->flip.tqueue.routine((void *)tty);
            if (tty->flip.count >= TTY_FLIPBUF_SIZE) {
                rc = 1;
                break;
            }
        }

		ch = plser_readb(port, SER_DATA);
		port->icount.rx++;
		flg = TTY_NORMAL;

		/*
		 * note that the error handling code is
		 * out of the main execution path
		 */
		if (status & (ST_PARITY|ST_FRAME|ST_OVERRUN)) {
        	if (status & ST_PARITY)
		        port->icount.parity++;
	        else if (status & ST_FRAME)
		        port->icount.frame++;
	        if (status & ST_OVERRUN)
		        port->icount.overrun++;

	        if (status & port->ignore_status_mask) {
		        if (++ignored > 100) {
                    rc = 1;
			        break;
                }
            }
            goto ignore_char;

	        status &= port->read_status_mask;

	        if (status & ST_PARITY)
		        flg = TTY_PARITY;
	        else if (status & ST_FRAME)
		        flg = TTY_FRAME;

        	if (status & ST_OVERRUN) {
        		/*
		         * overrun does *not* affect the character
		         * we read from the FIFO
		         */
		        *tty->flip.flag_buf_ptr++ = flg;
		        *tty->flip.char_buf_ptr++ = ch;
		        tty->flip.count++;
		        if (tty->flip.count >= TTY_FLIPBUF_SIZE)
			        goto ignore_char;
		        ch = 0;
		        flg = TTY_OVERRUN;
	        }
#ifdef SUPPORT_SYSRQ
	        info->sysrq = 0;
#endif
        }

		if (uart_handle_sysrq_char(info, ch, regs))
			goto ignore_char;

        *tty->flip.flag_buf_ptr++ = flg;
		*tty->flip.char_buf_ptr++ = ch;
		tty->flip.count++;

	ignore_char:
		status = plser_readb(port, SER_STATUS);
	}

	tty_flip_buffer_push(tty);
	return rc;

}

static inline int plser_tx_available(struct uart_port *port)
{
    int status;
    status = plser_readb(port, SER_STATUS); //  & port->read_status_mask;

    return ((status & ST_TX_FULL) == 0); // && (status & ST_CTS) == ST_CTS);

}

static int plser_tx_chars(struct uart_info *info)
{
	struct uart_port *port = info->port;

	if (port->x_char) {
        if (! plser_tx_available(port)) {
            return 1;
        }

		plser_writeb(port, port->x_char, SER_DATA);
		port->icount.tx++;
		port->x_char = 0;
		return 0;
	}

	if (info->xmit.head == info->xmit.tail
	    || info->tty->stopped
	    || info->tty->hw_stopped) {
		plser_stop_tx(info->port, 0);
		return 0;
	}

	/*
	 * Tried using FIFO (not checking TNF) for fifo fill:
	 * still had the '4 bytes repeated' problem.
	 */
	while (plser_tx_available(port)) {
        plser_writeb(port, info->xmit.buf[info->xmit.tail], SER_DATA);
		info->xmit.tail = (info->xmit.tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (info->xmit.head == info->xmit.tail)
			break;
	}

	if (CIRC_CNT(info->xmit.head, info->xmit.tail, UART_XMIT_SIZE) <
			WAKEUP_CHARS)
		uart_event(info, EVT_WRITE_WAKEUP);

	if (info->xmit.head == info->xmit.tail)
		plser_stop_tx(info->port, 0);
    else
        return 1;

    return 0;

}

static void plser_int(int irq, void *dev_id, struct pt_regs *regs)
{
	struct uart_info *info = dev_id;
	struct uart_port *port = info->port;
	unsigned int status, pass_counter = 0;
    int rc = 0;


	status = plser_readb(port, SER_STATUS); //  & port->read_status_mask;

    /* avoid reentery, should get latch firstly */
    if (test_and_set_bit(irq, &irq_latch))
        return;

	do {
		if (status & ST_RX_READY)
			rc |= plser_rx_chars(info, regs);

		if (status & ST_BREAK)
			port->icount.brk++;

        rc |= plser_tx_chars(info);

		if (pass_counter++ > 256 || rc)
			break;

		status = plser_readb(port, SER_STATUS); // & port->read_status_mask;

	} while(status & ST_RX_READY);

    /* release latch */
    clear_bit(irq, &irq_latch);
    clear_bit(irq, &queue_latch);

    status = plser_readb(port, SER_STATUS);

    if (pass_counter > 256 || rc || ((status & ST_TX_EMPTY) == 0) || (status & ST_RX_READY))
        plser_delay_tx_task(port);
}

/*
 * Return TIOCSER_TEMT when transmitter is not busy.
 */
static u_int plser_tx_empty(struct uart_port *port)
{
    int status;
    status = plser_readb(port, SER_STATUS);
    plser_delay_tx_task(port);

	return (status & ST_TX_EMPTY) ? TIOCSER_TEMT : 0;
}

static u_int plser_get_mctrl(struct uart_port *port)
{
	return ST_CTS | TIOCM_DSR | TIOCM_CAR;
}

static void plser_set_mctrl(struct uart_port *port, u_int mctrl)
{
    /* no modem control can be done */
}

/*
 * Interrupts always disabled.
 */
static void plser_break_ctl(struct uart_port *port, int break_state)
{
    static int last_break_state = 0;

    if (break_state == last_break_state)
        return;

    plser_writeb(port, 0xff, SER_SRB_GEN);
    last_break_state = break_state;
}


static int plser_startup(struct uart_port *port, struct uart_info *info)
{
	int retval;

	/*
	 * Allocate the IRQ
	 */
    clear_bit(port->irq, &irq_latch);
	retval = request_irq(port->irq, plser_int, 0, "plser", info);
	if (retval)
		return retval;

    clear_bit(port->irq, &queue_latch);
    port->read_status_mask |= ST_RX_READY;

    /* start the uart_info pointer so we can reference it in plser_start_tx */
    memcpy(port->unused, &info, sizeof(void *));


    queue_task(&plser_task, &tq_immediate);
    mark_bh(IMMEDIATE_BH);

	return 0;
}

static void plser_shutdown(struct uart_port *port, struct uart_info *info)
{
	/*
	 * Free the interrupt
	 */
    set_bit(port->irq, &queue_latch);
	free_irq(port->irq, info);
}


static void plser_change_speed(struct uart_port *port, u_int cflag, u_int iflag, u_int quot)
{
	unsigned long flags;

	/* byte size and parity */
    if ((cflag & CSIZE) != CS8)
        printk("pl serial only support 8 bit size\n");

    if (!((cflag & PARENB) && !(cflag & PARODD)))
        printk("pl serial only support even parity\n");

	if (cflag & CSTOPB)
        printk("pl serial only support 1 stop bit\n");

	port->read_status_mask |= ST_OVERRUN;
	if (iflag & INPCK)
		port->read_status_mask |= ST_FRAME | ST_PARITY;
	if (iflag & (BRKINT | PARMRK))
		port->read_status_mask |= ST_BREAK;


    /*
     * Characters to ignore
     */
    port->ignore_status_mask = 0;
    if (iflag & IGNPAR)
        port->ignore_status_mask |= ST_PARITY | ST_FRAME;
    if (iflag & IGNBRK) {
        port->ignore_status_mask |= ST_BREAK;
        /*
         * If we're ignoring parity and break indicators,
         * ignore overruns too (for real raw support).
         */
        if (iflag & IGNPAR)
            port->ignore_status_mask |= ST_OVERRUN;
    }

	while (!(plser_readb(port, SER_STATUS) & ST_TX_EMPTY));

#if 0
	/* set the baud rate */
    plser_writeb(port, 0, SER_CONTROL);     /* S/W set baudrate */
    plser_writeb(port, 0xff, SER_CON_RST);  /* console reset */
    plser_writew(port, quot, SER_PERIOD);   /* set baud rate */
    plser_writeb(port, 0x05, SER_CONTROL);  /* S/W set baudrate detected */
#endif

    plser_delay_tx_task(port);
}



static const char *plser_type(struct uart_port *port)
{
	return port->type == PORT_PLSER ? "PLSER" : NULL;
}

/*
 * do nothing for port release
 */
static void plser_release_port(struct uart_port *port)
{
	return;
}

/*
 * memory mapping has done in arch.c
 */
static int plser_request_port(struct uart_port *port)
{
	return 0;
}

/*
 * Configure/autoconfigure the port.
 */
static void plser_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE && plser_request_port(port) == 0)
		port->type = PORT_PLSER;
}

/*
 * Verify the new serial_struct (for TIOCSSERIAL).
 * The only change we allow are to the flags and type, and
 */
static int plser_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int ret = 0;
	if (ser->type != PORT_UNKNOWN && ser->type != PORT_PLSER)
		ret = -EINVAL;
	if (port->irq != ser->irq)
		ret = -EINVAL;
	if (ser->io_type != SERIAL_IO_MEM)
		ret = -EINVAL;
	if (port->uartclk / 16 != ser->baud_base)
		ret = -EINVAL;
	if ((void *)port->mapbase != ser->iomem_base)
		ret = -EINVAL;
	if (port->iobase != ser->port)
		ret = -EINVAL;
	if (ser->hub6 != 0)
		ret = -EINVAL;
	return ret;
}

static struct uart_ops plser_pops = {
	tx_empty:	        plser_tx_empty,
	set_mctrl:	        plser_set_mctrl,
	get_mctrl:	        plser_get_mctrl,
	stop_tx:	        plser_stop_tx,
	start_tx:	        plser_start_tx,
	stop_rx:	        plser_stop_rx,
	enable_ms:	        plser_enable_ms,
	break_ctl:	        plser_break_ctl,
	startup:	        plser_startup,
	shutdown:	        plser_shutdown,
	change_speed:	    plser_change_speed,
	type:		        plser_type,
	release_port:	    plser_release_port,
	request_port:	    plser_request_port,
	config_port:	    plser_config_port,
	verify_port:	    plser_verify_port,
};

static struct uart_port plser_ports[NR_PORTS];

/*
 * Setup the Prolific serial ports.
 */
static void plser_init_ports(void)
{
	static int first = 1;

	if (!first)
		return;
	first = 0;

    plser_ports[0].uartclk  = 96*1000*1000*16; /* pl_get_pci_hz()*16; */
    plser_ports[0].ops      = &plser_pops;
	plser_ports[0].fifosize = 4;

    plser_task.routine = plser_task_action;
    plser_task.data = (void *)&plser_ports[0];

}


void __init plser_register_uart(int idx, int port)
{
	if (idx >= NR_PORTS) {
		printk(KERN_ERR "%s: bad index number %d\n", __FUNCTION__ , idx);
		return;
	}

    if (port != 1) {
 		printk(KERN_ERR "%s: bad port number %d\n", __FUNCTION__, port);
        return;
    }

    plser_ports[0].membase = (void *)PL_CONSOLE_BASE;
	plser_ports[0].mapbase = virt_to_phys((void *)PL_CONSOLE_BASE);
	plser_ports[0].irq     = IRQ_PL_CONSOLE;
    plser_ports[0].iotype  = SERIAL_IO_MEM;
	plser_ports[0].flags   = ASYNC_BOOT_AUTOCONF;

}


/*
 * Interrupts are disabled on entering
 */
static void plser_console_write(struct console *co, const char *s, u_int count)
{
	struct uart_port *port = plser_ports + co->index;
	const char *e = s+count;
    int status;

	/*
	 *	disable interrupts
	 */
    disable_irq(port->irq);
    for (; s < e; s++) {
        do {
            status = plser_readb(port, SER_STATUS);
        } while(status & ST_TX_FULL);
        plser_writeb(port, *s, SER_DATA);
        if (*s == '\n') {
            do {
                status = plser_readb(port, SER_STATUS);
            } while(status & ST_TX_FULL);
            plser_writeb(port, '\r', SER_DATA);
        }
    }

	/*
	 *	Finally, wait for transmitter to become empty
	 */
	do {
        status = plser_readb(port, SER_STATUS);
	} while (!(status & ST_TX_EMPTY));

	enable_irq(port->irq);
    plser_delay_tx_task(port);
}

static kdev_t plser_console_device(struct console *co)
{
	return MKDEV(SERIAL_PLSER_MAJOR, MINOR_START + co->index);
}

/*
 * If the port was already initialised (eg, by a boot loader), try to determine
 * the current setup.
 */

static int default_bauds[] __initdata = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800};

static void __init
plser_console_get_options(struct uart_port *port, int *baud, int *parity, int *bits)
{
    u_int quot;
    int abaud;
    int i;
    /* This function should be modified to get exactly option from hardware */
    quot = plser_readw(port, SER_PERIOD);
    if (quot == 0) {
        abaud = 38400;
    } else {
        abaud = port->uartclk/(16*quot);
        *baud = abaud;
        for (i = 0; i < sizeof(default_bauds)/sizeof(default_bauds[0]); i++) {
            if ((abaud >= default_bauds[i]) &&  (abaud < default_bauds[i] + quot)) {
                *baud = default_bauds[i];
                break;
            }
        }
    }

	*parity = 'e';
    *bits = 8;
}

static int __init
plser_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 230400;
	int bits = 8;
	int parity = 'e';   /* even parity */
	int flow = 'r';     /* rts flow control */
	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	port = uart_get_console(plser_ports, NR_PORTS, co);

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	else
		plser_console_get_options(port, &baud, &parity, &bits);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct console plser_console = {
	name:		"tts/0",
	write:		plser_console_write,
	device:		plser_console_device,
	setup:		plser_console_setup,
	flags:		CON_PRINTBUFFER,
	index:		-1,
};


void __init plser_rs_console_init(void)
{
	plser_init_ports();
	register_console(&plser_console);
    printk("plser console driver v2.0.0\n");
}


static struct uart_driver plser_reg = {
	owner:			    THIS_MODULE,
	normal_major:		SERIAL_PLSER_MAJOR,
#ifdef CONFIG_DEVFS_FS
	normal_name:		"tts/%d",
	callout_name:		"cua/%d",
#else
	normal_name:		"ttyS",
	callout_name:		"cua",
#endif
	normal_driver:		&normal,
	callout_major:		CALLOUT_PLSER_MAJOR,
	callout_driver:		&callout,
	table:			    plser_table,
	termios:		    plser_termios,
	termios_locked:		plser_termios_locked,
	minor:			    MINOR_START,
	nr:			        NR_PORTS,
	port:			    plser_ports,
	cons:			    &plser_console,
};



static int __init plser_init(void)
{
    int rc;
    plser_init_ports();
    rc = uart_register_driver(&plser_reg);
    return rc;
}

static void __exit plser_exit(void)
{
    uart_unregister_driver(&plser_reg);
}


module_init(plser_init);
module_exit(plser_exit);

EXPORT_NO_SYMBOLS;

MODULE_AUTHOR("Jedy Wei, Prolific Technology Inc.");
MODULE_DESCRIPTION("Prolific console port driver");
MODULE_LICENSE("GPL");

