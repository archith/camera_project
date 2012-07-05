/*
 *          mxser.c  -- MOXA Smartio/Industio family multiport serial driver.
 *
 *      Copyright (C) 1999-2001  Moxa Technologies (support@moxa.com.tw).
 *
 *      This code is loosely based on the Linux serial driver, written by
 *      Linus Torvalds, Theodore T'so and others.
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *	Original release	10/26/00
 *
 *	02/06/01	Support MOXA Industio family boards.
 *	02/06/01	Support TIOCGICOUNT.
 *	02/06/01	Fix the problem for connecting to serial mouse.
 *	02/06/01	Fix the problem for H/W flow control.
 *	02/06/01	Fix the compling warning when CONFIG_PCI
 *			don't be defined.
 */

#ifdef 		MODVERSIONS
#ifndef 	MODULE
#define 	MODULE
#endif
#endif

#ifdef MODULE
#include <linux/config.h>
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#else
#define	MOD_INC_USE_COUNT
#define MOD_DEC_USE_COUNT
#endif
#include <linux/version.h>
#include <linux/autoconf.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_reg.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/smp_lock.h>
#include <linux/delay.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/segment.h>
#include <asm/bitops.h>

#define	VERSION_CODE(ver,rel,seq)	((ver << 16) | (rel << 8) | seq)

#ifdef CONFIG_PCI
#include <linux/pci.h>
#endif

#define	MXSER_VERSION	"1.6.1"
#define	MXSERMAJOR	 30
#define	MXSERCUMAJOR	 35

#ifdef CONFIG_PCI
#if (LINUX_VERSION_CODE < VERSION_CODE(2,1,0))
#include <linux/bios32.h>
#endif
#include <linux/pci.h>
#endif /* ENABLE_PCI */

#if (LINUX_VERSION_CODE < VERSION_CODE(2,1,0))

#define copy_from_user memcpy_fromfs
#define copy_to_user memcpy_tofs
#define put_to_user(arg1, arg2) put_fs_long(arg1, (unsigned long *)arg2)
#define get_from_user(arg1, arg2) arg1 = get_fs_long((unsigned long *)arg2)
#define schedule_timeout(x) {current->timeout = jiffies + (x); schedule();}
#define signal_pending(x) ((x)->signal & ~(x)->blocked)
#else
#include <asm/uaccess.h>
#define put_to_user(arg1, arg2) put_user(arg1, (unsigned long *)arg2)
#define get_from_user(arg1, arg2) get_user(arg1, (unsigned int *)arg2)
#endif

#define	MXSER_EVENT_TXLOW	 1
#define	MXSER_EVENT_HANGUP	 2

#define SERIAL_DO_RESTART
#define MXSER_BOARDS		4	/* Max. boards */
#define MXSER_PORTS		32	/* Max. ports */
#define MXSER_PORTS_PER_BOARD	8	/* Max. ports per board*/
#define MXSER_ISR_PASS_LIMIT	256

#define	MXSER_ERR_IOADDR	-1
#define	MXSER_ERR_IRQ		-2
#define	MXSER_ERR_IRQ_CONFLIT	-3
#define	MXSER_ERR_VECTOR	-4

#define SERIAL_TYPE_NORMAL	1
#define SERIAL_TYPE_CALLOUT	2

#define WAKEUP_CHARS		256

#define UART_MCR_AFE		0x20
#define UART_LSR_SPECIAL	0x1E

#define PORTNO(x)	(MINOR((x)->device) - (x)->driver.minor_start)

#define RELEVANT_IFLAG(iflag)	(iflag & (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK|IXON|IXOFF))

#define IRQ_T(info) ((info->flags & ASYNC_SHARE_IRQ) ? SA_SHIRQ : SA_INTERRUPT)

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

/*
// Patrick added start 20040802
#ifdef CONFIG_UCLINUX
#define voutb(v,p) 	outb((v),(p))
#define vinb(p) 	inb((p))
#else
#define voutb(v,p) 	outb((v),IO_ADDRESS((p)))
#define vinb(p) 	inb(IO_ADDRESS((p)))
#endif
// Patrick added end 20040802
*/
/*
 *	Define the Moxa PCI vendor and device IDs.
 */
#ifndef	PCI_VENDOR_ID_MOXA
#define	PCI_VENDOR_ID_MOXA	0x1393
#endif

#ifndef PCI_DEVICE_ID_C168
#define PCI_DEVICE_ID_C168	0x1680
#endif

#ifndef PCI_DEVICE_ID_C104
#define PCI_DEVICE_ID_C104	0x1040
#endif

#ifndef PCI_DEVICE_ID_CP132
#define PCI_DEVICE_ID_CP132	0x1320
#endif

#ifndef PCI_DEVICE_ID_CP114
#define PCI_DEVICE_ID_CP114	0x1141
#endif

#ifndef PCI_DEVICE_ID_CT114
#define PCI_DEVICE_ID_CT114	0x1140
#endif

#ifndef PCI_DEVICE_ID_CP102
#define PCI_DEVICE_ID_CP102	0x1020
#endif

#ifndef PCI_DEVICE_ID_CP104U
#define PCI_DEVICE_ID_CP104U	0x1041
#endif

#ifndef PCI_DEVICE_ID_CP168U
#define PCI_DEVICE_ID_CP168U	0x1681
#endif

#ifndef PCI_DEVICE_ID_CP168N
#define PCI_DEVICE_ID_CP168N	0x1682
#endif

#ifndef PCI_DEVICE_ID_RC7000
#define PCI_DEVICE_ID_RC7000	0x0001
#endif

#ifndef PCI_DEVICE_ID_CP132U
#define PCI_DEVICE_ID_CP132U	0x1321
#endif

#ifndef PCI_DEVICE_ID_CP134U
#define PCI_DEVICE_ID_CP134U	0x1340
#endif

#define C168_ASIC_ID    1
#define C104_ASIC_ID    2
#define C102_ASIC_ID	0xB
#define CI132_ASIC_ID	4
#define CI134_ASIC_ID	3
#define CI104J_ASIC_ID  5

enum	{
	MXSER_BOARD_C168_ISA = 1,
	MXSER_BOARD_C104_ISA,
        MXSER_BOARD_CI104J,
	MXSER_BOARD_C168_PCI,
	MXSER_BOARD_C104_PCI,
	MXSER_BOARD_C102_ISA,
	MXSER_BOARD_CI132,
	MXSER_BOARD_CI134,
	MXSER_BOARD_CP132,
	MXSER_BOARD_CP114,
	MXSER_BOARD_CT114,
	MXSER_BOARD_CP102,
	MXSER_BOARD_CP104U,
	MXSER_BOARD_CP168U,
	MXSER_BOARD_CP132U,
	MXSER_BOARD_CP134U,
	MXSER_BOARD_CP168N,
	MXSER_BOARD_RC7000,
};

static char *mxser_brdname[] = {
	"C168 series",
	"C104 series",
        "CI-104J series",
	"C168H/PCI series",
	"C104H/PCI series",
	"C102 series",
	"CI-132 series",
	"CI-134 series",
	"CP-132 series",
	"CP-114 series",
	"CT-114 series",
	"CP-102 series",
	"CP-104U series",
	"CP-168U series",
	"CP-132U series",
	"CP-134U series",
	"CP-168N series",
	"Moxa UC7000 Serial",
};

static int mxser_numports[] = {
	8,	// C168-ISA
	4,	// C104-ISA
	4,	// CI104J
	8,	// C168-PCI
	4,	// C104-PCI
	2,	// C102-ISA
	2,	// CI132
	4,	// CI134
	2,	// CP132
	4,	// CP114
	4,	// CT114
	2,	// CP102
	4,	// CP104U
	8,	// CP168U
	2,	// CP132U
	4,	// CP134U
	8,	// CP168N
	8,	// RC7000
};

/*
 *	MOXA ioctls
 */
#define MOXA			0x400
#define MOXA_GETDATACOUNT     (MOXA + 23)
#define	MOXA_GET_CONF         (MOXA + 35)
#define MOXA_DIAGNOSE         (MOXA + 50)
#define MOXA_CHKPORTENABLE    (MOXA + 60)
#define MOXA_HighSpeedOn      (MOXA + 61)
#define MOXA_GET_MAJOR        (MOXA + 63)
#define MOXA_GET_CUMAJOR      (MOXA + 64)
#define MOXA_GETMSTATUS       (MOXA + 65)

// following add by Victor Yu. 01-05-2004
#define MOXA_SET_OP_MODE      (MOXA + 66)
#define MOXA_GET_OP_MODE      (MOXA + 67)

#define RS232_MODE		0
#define RS485_2WIRE_MODE	1
#define RS422_MODE		2
#define RS485_4WIRE_MODE	3
#define OP_MODE_MASK		3
// above add by Victor Yu. 01-05-2004

#define TTY_THRESHOLD_THROTTLE  128
#define ID1_XMIT_SIZE		128
#define LO_WATER	 	(TTY_FLIPBUF_SIZE)
#define HI_WATER		(TTY_FLIPBUF_SIZE*2*3/4)
//#define LO_WATER	 	0
//#define HI_WATER	 	(TTY_THRESHOLD_THROTTLE)

// added by James. 03-11-2004.
#define MOXA_SDS_GETICOUNTER  (MOXA + 68)
#define MOXA_SDS_RSTICOUNTER  (MOXA + 69)
// (above) added by James.

#define MOXA_ASPP_OQUEUE  (MOXA + 70)
#define MOXA_ASPP_SETBAUD (MOXA + 71)
#define MOXA_ASPP_GETBAUD (MOXA + 72)
#define MOXA_ASPP_MON     (MOXA + 73)
#define MOXA_ASPP_LSTATUS (MOXA + 74)
#define MOXA_ASPP_MON_EXT (MOXA + 75)
#define MOXA_SET_BAUD_METHOD	(MOXA + 76)

#define NPPI_NOTIFY_PARITY	0x01
#define NPPI_NOTIFY_FRAMING	0x02
#define NPPI_NOTIFY_HW_OVERRUN	0x04
#define NPPI_NOTIFY_SW_OVERRUN	0x08
#define NPPI_NOTIFY_BREAK	0x10

#define NPPI_NOTIFY_CTSHOLD         0x01    // Tx hold by CTS low
#define NPPI_NOTIFY_DSRHOLD         0x02    // Tx hold by DSR low
#define NPPI_NOTIFY_XOFFHOLD        0x08    // Tx hold by Xoff received
#define NPPI_NOTIFY_XOFFXENT        0x10    // Xoff Sent
#ifdef CONFIG_PCI

typedef struct {
	unsigned short	vendor_id;
	unsigned short	device_id;
	unsigned short	board_type;
}	mxser_pciinfo;

static mxser_pciinfo	mxser_pcibrds[] = {
	{PCI_VENDOR_ID_MOXA,PCI_DEVICE_ID_C168 ,MXSER_BOARD_C168_PCI},
	{PCI_VENDOR_ID_MOXA,PCI_DEVICE_ID_C104 ,MXSER_BOARD_C104_PCI},
	{PCI_VENDOR_ID_MOXA,PCI_DEVICE_ID_CP132 ,MXSER_BOARD_CP132},
	{PCI_VENDOR_ID_MOXA,PCI_DEVICE_ID_CP114 ,MXSER_BOARD_CP114},
	{PCI_VENDOR_ID_MOXA,PCI_DEVICE_ID_CT114 ,MXSER_BOARD_CT114},
	{PCI_VENDOR_ID_MOXA,PCI_DEVICE_ID_CP102 ,MXSER_BOARD_CP102},
	{PCI_VENDOR_ID_MOXA,PCI_DEVICE_ID_CP104U ,MXSER_BOARD_CP104U},
	{PCI_VENDOR_ID_MOXA,PCI_DEVICE_ID_CP168U ,MXSER_BOARD_CP168U},
	{PCI_VENDOR_ID_MOXA,PCI_DEVICE_ID_CP132U ,MXSER_BOARD_CP132U},
	{PCI_VENDOR_ID_MOXA,PCI_DEVICE_ID_CP134U ,MXSER_BOARD_CP134U},
	{PCI_VENDOR_ID_MOXA,PCI_DEVICE_ID_CP168N ,MXSER_BOARD_CP168N},
	{PCI_VENDOR_ID_MOXA,PCI_DEVICE_ID_RC7000 ,MXSER_BOARD_RC7000},
};


#endif

typedef struct _moxa_pci_info {
	unsigned short busNum;
	unsigned short devNum;
struct pci_dev	*pdev;	// add by Victor Yu. 06-23-2003
} moxa_pci_info;

static int ioaddr[MXSER_BOARDS]={0,0,0,0};
static int ttymajor=MXSERMAJOR;
static int calloutmajor=MXSERCUMAJOR;
static int verbose=0;

#ifdef MODULE
/* Variables for insmod */

# if (LINUX_VERSION_CODE > VERSION_CODE(2,1,11))
MODULE_AUTHOR("William Chen");
MODULE_DESCRIPTION("MOXA Smartio/Industio Family Multiport Board Device Driver");
MODULE_PARM(ioaddr,     "1-4i");
MODULE_PARM(ttymajor,        "i");
MODULE_PARM(calloutmajor,    "i");
MODULE_PARM(verbose,        "i");
# endif

#endif /* MODULE */



struct mxser_log {
	int	tick;
	unsigned long	rxcnt[MXSER_PORTS];
	unsigned long	txcnt[MXSER_PORTS];
};


struct mxser_mon{
        unsigned long   rxcnt;
        unsigned long   txcnt;
		unsigned long	up_rxcnt;
		unsigned long	up_txcnt;
        int             modem_status;
        unsigned char   hold_reason;
};

struct mxser_mon_ext
{
	unsigned long rx_cnt[32];
	unsigned long tx_cnt[32];
	unsigned long up_rxcnt[32];
	unsigned long up_txcnt[32];
	int	modem_status[32];
};
struct mxser_hwconf {
	int		board_type;
	int		ports;
	int		irq;
	int		vector;
	int		vector_mask;
	int		uart_type;
	int		ioaddr[MXSER_PORTS_PER_BOARD];
	int		baud_base[MXSER_PORTS_PER_BOARD];
	moxa_pci_info	pciInfo;
	int		IsMoxaMustChipFlag;	// add by Victor Yu. 08-30-2002
	int		MaxCanSetBaudRate[MXSER_PORTS_PER_BOARD];	// add by Victor Yu. 09-04-2002
	int		opmode_ioaddr[MXSER_PORTS_PER_BOARD];	// add by Victor Yu. 01-05-2004
};

struct mxser_struct { 		// each port setttings
	int			port;
	int			base;		/* port base address */
	int			irq;		/* port using irq no. */
	int			vector; 	/* port irq vector */
	int			vectormask;	/* port vector mask */
	int			rx_trigger;	/* Rx fifo trigger level */
	int			baud_base;	/* max. speed */
	int			flags;		/* defined in tty.h */
	int			type;		/* UART type */
	struct tty_struct *	tty;
	int			read_status_mask;
	int			ignore_status_mask;
	int			xmit_fifo_size;
	int			custom_divisor;
	int			x_char; 	/* xon/xoff character */
	int			close_delay;
	unsigned short		closing_wait;
	int			IER;		/* Interrupt Enable Register */
	int			MCR;		/* Modem control register */
	unsigned long		event;
	int			count;		/* # of fd on device */
	int			blocked_open;	/* # of blocked opens */
	long			session;	/* Session of opening process */
	long			pgrp;		/* pgrp of opening process */
	unsigned char		*xmit_buf;
	int			xmit_head;
	int			xmit_tail;
	int			xmit_cnt;
	struct tq_struct	tqueue;
	struct termios		normal_termios;
	struct termios		callout_termios;
#if (LINUX_VERSION_CODE < VERSION_CODE(2,4,0))
	struct wait_queue	*open_wait;
	struct wait_queue	*close_wait;
	struct wait_queue	*delta_msr_wait;
#else
	wait_queue_head_t open_wait;
	wait_queue_head_t close_wait;
	wait_queue_head_t delta_msr_wait;
#endif
	struct async_icount	icount; 	/* kernel counters for the 4 input interrupts */
	int			timeout;
	int			IsMoxaMustChipFlag;	// add by Victor Yu. 08-30-2002
	int			MaxCanSetBaudRate;	// add by Victor Yu. 09-04-2002
	int		opmode_ioaddr;	// add by Victor Yu. 01-05-2004
	unsigned char		stop_rx;
	unsigned char       ldisc_stop_rx;
	long    realbaud;
	struct mxser_mon        mon_data;
	unsigned char           err_shadow;
};


struct mxser_mstatus{
       tcflag_t	cflag;
       int  	cts;
       int  	dsr;
       int  	ri;
       int  	dcd;
};

static  struct mxser_mstatus GMStatus[MXSER_PORTS];

static int mxserBoardCAP[MXSER_BOARDS]  = {
	0,0,0,0
       /*  0x180, 0x280, 0x200, 0x320   */
};


static struct tty_driver	mxvar_sdriver, mxvar_cdriver;
static int			mxvar_refcount;
static struct mxser_struct	mxvar_table[MXSER_PORTS];
static struct tty_struct *	mxvar_tty[MXSER_PORTS+1];
static struct termios * 	mxvar_termios[MXSER_PORTS+1];
static struct termios * 	mxvar_termios_locked[MXSER_PORTS+1];
static struct mxser_log 	mxvar_log;
static int			mxvar_diagflag;
static unsigned char mxser_msr[MXSER_PORTS+1];
static struct mxser_mon_ext mon_data_ext;
static int mxser_set_baud_method[MXSER_PORTS+1];
//static int                      moxaTimer_on;
//static struct timer_list        moxaTimer;
/*
 * mxvar_tmp_buf is used as a temporary buffer by serial_write. We need
 * to lock it in case the memcpy_fromfs blocks while swapping in a page,
 * and some other program tries to do a serial write at the same time.
 * Since the lock will only come under contention when the system is
 * swapping and available memory is low, it makes sense to share one
 * buffer across all the serial ports, since it significantly saves
 * memory if large numbers of serial ports are open.
 */
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,4,0))
static unsigned char *		mxvar_tmp_buf;
static struct semaphore 	mxvar_tmp_buf_sem;
#else
static unsigned char *		mxvar_tmp_buf = 0;
static struct semaphore 	mxvar_tmp_buf_sem = MUTEX;
#endif

/*
 * This is used to figure out the divisor speeds and the timeouts
 */
/*
static int mxvar_baud_table[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800,
	9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 0 };
*/
struct mxser_hwconf mxsercfg[MXSER_BOARDS];

/*
 * static functions:
 */

#ifdef MODULE
int		init_module(void);
void		cleanup_module(void);
#endif

static void 	mxser_getcfg(int board,struct mxser_hwconf *hwconf);
int		mxser_init(void);

//static void	mxser_poll(unsigned long);
static int	mxser_get_ISA_conf(int, struct mxser_hwconf *);
#ifdef CONFIG_PCI
static int      mxser_get_PCI_conf(int ,int ,int ,struct mxser_hwconf *);
#endif
static void	mxser_do_softint(void *);
static int	mxser_open(struct tty_struct *, struct file *);
static void	mxser_close(struct tty_struct *, struct file *);
static int	mxser_write(struct tty_struct *, int, const unsigned char *, int);
static int	mxser_write_room(struct tty_struct *);
static void	mxser_flush_buffer(struct tty_struct *);
static int	mxser_chars_in_buffer(struct tty_struct *);
static void	mxser_flush_chars(struct tty_struct *);
static void	mxser_put_char(struct tty_struct *, unsigned char);
static int	mxser_ioctl(struct tty_struct *, struct file *, uint, ulong);
static int	mxser_ioctl_special(unsigned int, unsigned long);
static void	mxser_throttle(struct tty_struct *);
static void	mxser_unthrottle(struct tty_struct *);
static void	mxser_set_termios(struct tty_struct *, struct termios *);
static void	mxser_stop(struct tty_struct *);
static void	mxser_start(struct tty_struct *);
static void	mxser_hangup(struct tty_struct *);
static void mxser_rs_break(struct tty_struct *, int);
static void	mxser_interrupt(int, void *, struct pt_regs *);
static inline void mxser_receive_chars(struct mxser_struct *, int *);
static inline void mxser_transmit_chars(struct mxser_struct *);
static inline void mxser_check_modem_status(struct mxser_struct *, int);
static int	mxser_block_til_ready(struct tty_struct *, struct file *, struct mxser_struct *);
static int	mxser_startup(struct mxser_struct *);
static void	mxser_shutdown(struct mxser_struct *);
static int	mxser_change_speed(struct mxser_struct *, struct termios *old_termios);
static int	mxser_get_serial_info(struct mxser_struct *, struct serial_struct *);
static int	mxser_set_serial_info(struct mxser_struct *, struct serial_struct *);
static int	mxser_get_lsr_info(struct mxser_struct *, unsigned int *);
static void	mxser_send_break(struct mxser_struct *, int);
static int	mxser_get_modem_info(struct mxser_struct *, unsigned int *);
static int	mxser_set_modem_info(struct mxser_struct *, unsigned int, unsigned int *);
static int mxser_set_baud(struct mxser_struct *info, long newspd);
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
static void 	mxser_wait_until_sent(struct tty_struct *tty, int timeout);
#endif

static void mxser_startrx(struct tty_struct * tty);
static void mxser_stoprx(struct tty_struct * tty);

//
// follwoing is modified by Victor Yu. 08-15-2002
//
// follow just for Moxa Must chip define.
//
// when LCR register (offset 0x03) write following value,
// the Must chip will enter enchance mode. And write value
// on EFR (offset 0x02) bit 6,7 to change bank.
#define MOXA_MUST_ENTER_ENCHANCE	0xBF

// when enhance mode enable, access on general bank register
#define MOXA_MUST_GDL_REGISTER		0x07
#define MOXA_MUST_GDL_MASK		0x7F
#define MOXA_MUST_GDL_HAS_BAD_DATA	0x80

#define MOXA_MUST_LSR_RERR		0x80	// error in receive FIFO
// enchance register bank select and enchance mode setting register
// when LCR register equal to 0xBF
#define MOXA_MUST_EFR_REGISTER		0x02
// enchance mode enable
#define MOXA_MUST_EFR_EFRB_ENABLE	0x10
// enchance reister bank set 0, 1, 2
#define MOXA_MUST_EFR_BANK0		0x00
#define MOXA_MUST_EFR_BANK1		0x40
#define MOXA_MUST_EFR_BANK2		0x80
#define MOXA_MUST_EFR_BANK3		0xC0
#define MOXA_MUST_EFR_BANK_MASK		0xC0

// set XON1 value register, when LCR=0xBF and change to bank0
#define MOXA_MUST_XON1_REGISTER		0x04

// set XON2 value register, when LCR=0xBF and change to bank0
#define MOXA_MUST_XON2_REGISTER		0x05

// set XOFF1 value register, when LCR=0xBF and change to bank0
#define MOXA_MUST_XOFF1_REGISTER	0x06

// set XOFF2 value register, when LCR=0xBF and change to bank0
#define MOXA_MUST_XOFF2_REGISTER	0x07

#define MOXA_MUST_RBRTL_REGISTER	0x04
#define MOXA_MUST_RBRTH_REGISTER	0x05
#define MOXA_MUST_RBRTI_REGISTER	0x06
#define MOXA_MUST_THRTL_REGISTER	0x07
#define MOXA_MUST_ENUM_REGISTER		0x04
#define MOXA_MUST_HWID_REGISTER		0x05
#define MOXA_MUST_ECR_REGISTER		0x06
#define MOXA_MUST_CSR_REGISTER		0x07

// good data mode enable
#define MOXA_MUST_FCR_GDA_MODE_ENABLE	0x20
// only good data put into RxFIFO
#define MOXA_MUST_FCR_GDA_ONLY_ENABLE	0x10

// enable CTS interrupt
#define MOXA_MUST_IER_ECTSI		0x80
// eanble RTS interrupt
#define MOXA_MUST_IER_ERTSI		0x40
// enable Xon/Xoff interrupt
#define MOXA_MUST_IER_XINT		0x20
// enable GDA interrupt
#define MOXA_MUST_IER_EGDAI		0x10

#define MOXA_MUST_RECV_ISR		(UART_IER_RDI | MOXA_MUST_IER_EGDAI)

// GDA interrupt pending
#define MOXA_MUST_IIR_GDA		0x1C
#define MOXA_MUST_IIR_RDA		0x04
#define MOXA_MUST_IIR_RTO		0x0C
#define MOXA_MUST_IIR_LSR		0x06

// recieved Xon/Xoff or specical interrupt pending
#define MOXA_MUST_IIR_XSC		0x10

// RTS/CTS change state interrupt pending
#define MOXA_MUST_IIR_RTSCTS		0x20
#define MOXA_MUST_IIR_MASK		0x3E

#define MOXA_MUST_MCR_XON_FLAG		0x40
#define MOXA_MUST_MCR_XON_ANY		0x80
#define MOXA_MUST_MCR_TX_XON		0x08
#define MOXA_MUST_HARDWARE_ID		0x01
#define MOXA_MUST_HARDWARE_ID1		0x02

// software flow control on chip mask value
#define MOXA_MUST_EFR_SF_MASK		0x0F
// send Xon1/Xoff1
#define MOXA_MUST_EFR_SF_TX1		0x08
// send Xon2/Xoff2
#define MOXA_MUST_EFR_SF_TX2		0x04
// send Xon1,Xon2/Xoff1,Xoff2
#define MOXA_MUST_EFR_SF_TX12		0x0C
// don't send Xon/Xoff
#define MOXA_MUST_EFR_SF_TX_NO		0x00
// Tx software flow control mask
#define MOXA_MUST_EFR_SF_TX_MASK	0x0C
// don't receive Xon/Xoff
#define MOXA_MUST_EFR_SF_RX_NO		0x00
// receive Xon1/Xoff1
#define MOXA_MUST_EFR_SF_RX1		0x02
// receive Xon2/Xoff2
#define MOXA_MUST_EFR_SF_RX2		0x01
// receive Xon1,Xon2/Xoff1,Xoff2
#define MOXA_MUST_EFR_SF_RX12		0x03
// Rx software flow control mask
#define MOXA_MUST_EFR_SF_RX_MASK	0x03

#define MOXA_MUST_MIN_XOFFLIMIT		112
#define MOXA_MUST_MIN_XONLIMIT		64
#define ID1_RX_TRIG			120

#ifndef UCHAR
typedef unsigned char	UCHAR;
#endif

#define CHECK_MOXA_MUST_XOFFLIMIT(info) { 	\
	if ( (info)->IsMoxaMustChipFlag && 	\
	 (info)->HandFlow.XoffLimit < MOXA_MUST_MIN_XOFFLIMIT ) {	\
		(info)->HandFlow.XoffLimit = MOXA_MUST_MIN_XOFFLIMIT;	\
		(info)->HandFlow.XonLimit = MOXA_MUST_MIN_XONLIMIT;	\
	}	\
}

#define ENABLE_MOXA_MUST_ENCHANCE_MODE(baseio) { \
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr |= MOXA_MUST_EFR_EFRB_ENABLE;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define DISABLE_MOXA_MUST_ENCHANCE_MODE(baseio) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_EFRB_ENABLE;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define SET_MOXA_MUST_XON1_VALUE(baseio, Value) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_BANK_MASK;	\
	__efr |= MOXA_MUST_EFR_BANK0;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb((UCHAR)(Value), (baseio)+MOXA_MUST_XON1_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define SET_MOXA_MUST_XON2_VALUE(baseio, Value) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_BANK_MASK;	\
	__efr |= MOXA_MUST_EFR_BANK0;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb((UCHAR)(Value), (baseio)+MOXA_MUST_XON2_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define SET_MOXA_MUST_XOFF1_VALUE(baseio, Value) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_BANK_MASK;	\
	__efr |= MOXA_MUST_EFR_BANK0;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb((UCHAR)(Value), (baseio)+MOXA_MUST_XOFF1_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define SET_MOXA_MUST_XOFF2_VALUE(baseio, Value) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_BANK_MASK;	\
	__efr |= MOXA_MUST_EFR_BANK0;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb((UCHAR)(Value), (baseio)+MOXA_MUST_XOFF2_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define SET_MOXA_MUST_RBRTL_VALUE(baseio, Value) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_BANK_MASK;	\
	__efr |= MOXA_MUST_EFR_BANK1;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb((UCHAR)(Value), (baseio)+MOXA_MUST_RBRTL_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define SET_MOXA_MUST_RBRTH_VALUE(baseio, Value) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_BANK_MASK;	\
	__efr |= MOXA_MUST_EFR_BANK1;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb((UCHAR)(Value), (baseio)+MOXA_MUST_RBRTH_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define SET_MOXA_MUST_RBRTI_VALUE(baseio, Value) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_BANK_MASK;	\
	__efr |= MOXA_MUST_EFR_BANK1;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb((UCHAR)(Value), (baseio)+MOXA_MUST_RBRTI_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define SET_MOXA_MUST_THRTL_VALUE(baseio, Value) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_BANK_MASK;	\
	__efr |= MOXA_MUST_EFR_BANK1;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb((UCHAR)(Value), (baseio)+MOXA_MUST_THRTL_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define MOXA_MUST_RBRL_VALUE	4
#define SET_MOXA_MUST_FIFO_VALUE(info) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((info)->base+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (info)->base+UART_LCR);	\
	__efr = inb((info)->base+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_BANK_MASK;	\
	__efr |= MOXA_MUST_EFR_BANK1;	\
	outb(__efr, (info)->base+MOXA_MUST_EFR_REGISTER);	\
	outb((UCHAR)0, (info)->base+MOXA_MUST_THRTL_REGISTER);	\
	if((info)->rx_trigger == 1){\
		outb((UCHAR)1, (info)->base+MOXA_MUST_RBRTH_REGISTER);	\
		outb((UCHAR)1, (info)->base+MOXA_MUST_RBRTI_REGISTER);	\
		outb((UCHAR)0,(info)->base+MOXA_MUST_RBRTL_REGISTER);	\
	}else{\
		outb((UCHAR)(MOXA_MUST_MIN_XOFFLIMIT), (info)->base+MOXA_MUST_RBRTH_REGISTER);	\
		outb((UCHAR)((info)->rx_trigger), (info)->base+MOXA_MUST_RBRTI_REGISTER);	\
		outb((UCHAR)(MOXA_MUST_MIN_XONLIMIT),(info)->base+MOXA_MUST_RBRTL_REGISTER);	\
	}\
	outb(__oldlcr, (info)->base+UART_LCR);	\
}



#define SET_MOXA_MUST_ENUM_VALUE(baseio, Value) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_BANK_MASK;	\
	__efr |= MOXA_MUST_EFR_BANK2;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb((UCHAR)(Value), (baseio)+MOXA_MUST_ENUM_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define GET_MOXA_MUST_HARDWARE_ID(baseio, pId) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_BANK_MASK;	\
	__efr |= MOXA_MUST_EFR_BANK2;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	*pId = inb((baseio)+MOXA_MUST_HWID_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define SET_MOXA_MUST_NO_SOFTWARE_FLOW_CONTROL(baseio) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_SF_MASK;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define SET_MOXA_MUST_JUST_TX_SOFTWARE_FLOW_CONTROL(baseio) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_SF_MASK;	\
	__efr |= MOXA_MUST_EFR_SF_TX1;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define ENABLE_MOXA_MUST_TX_SOFTWARE_FLOW_CONTROL(baseio) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_SF_TX_MASK;	\
	__efr |= MOXA_MUST_EFR_SF_TX1;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define DISABLE_MOXA_MUST_TX_SOFTWARE_FLOW_CONTROL(baseio) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_SF_TX_MASK;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define SET_MOXA_MUST_JUST_RX_SOFTWARE_FLOW_CONTROL(baseio) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_SF_MASK;	\
	__efr |= MOXA_MUST_EFR_SF_RX1;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define ENABLE_MOXA_MUST_RX_SOFTWARE_FLOW_CONTROL(baseio) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_SF_RX_MASK;	\
	__efr |= MOXA_MUST_EFR_SF_RX1;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define DISABLE_MOXA_MUST_RX_SOFTWARE_FLOW_CONTROL(baseio) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_SF_RX_MASK;	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define ENABLE_MOXA_MUST_TX_RX_SOFTWARE_FLOW_CONTROL(baseio) {	\
	UCHAR	__oldlcr, __efr;	\
	__oldlcr = inb((baseio)+UART_LCR);	\
	outb(MOXA_MUST_ENTER_ENCHANCE, (baseio)+UART_LCR);	\
	__efr = inb((baseio)+MOXA_MUST_EFR_REGISTER);	\
	__efr &= ~MOXA_MUST_EFR_SF_MASK;	\
	__efr |= (MOXA_MUST_EFR_SF_RX1|MOXA_MUST_EFR_SF_TX1);	\
	outb(__efr, (baseio)+MOXA_MUST_EFR_REGISTER);	\
	outb(__oldlcr, (baseio)+UART_LCR);	\
}

#define ENABLE_MOXA_MUST_XON_ANY_FLOW_CONTROL(baseio) {	\
	UCHAR	__oldmcr;	\
	__oldmcr = inb((baseio)+UART_MCR);	\
	__oldmcr |= MOXA_MUST_MCR_XON_ANY;	\
	outb(__oldmcr, (baseio)+UART_MCR);	\
}

#define DISABLE_MOXA_MUST_XON_ANY_FLOW_CONTROL(baseio) {	\
	UCHAR	__oldmcr;	\
	__oldmcr = inb((baseio)+UART_MCR);	\
	__oldmcr &= ~MOXA_MUST_MCR_XON_ANY;	\
	outb(__oldmcr, (baseio)+UART_MCR);	\
}

#define READ_MOXA_MUST_GDL(baseio)	inb((baseio)+MOXA_MUST_GDL_REGISTER)

static int CheckIsMoxaMust(int io)
{
	UCHAR	oldmcr, hwid;

	outb(0, io+UART_LCR);
	DISABLE_MOXA_MUST_ENCHANCE_MODE(io);
	oldmcr = inb(io+UART_MCR);
	outb(0, io+UART_MCR);
	SET_MOXA_MUST_XON1_VALUE(io, 0x11);
	if ( (hwid=inb(io+UART_MCR)) != 0 ) {
		outb(oldmcr, io+UART_MCR);
		return(0);
	}
	GET_MOXA_MUST_HARDWARE_ID(io, &hwid);
	if ( hwid != MOXA_MUST_HARDWARE_ID && hwid != MOXA_MUST_HARDWARE_ID1 )
		return(0);
	//return(1);	// mask by Victor Yu. 02-10-2004
	return((int)hwid);	// add by Victor Yu. 02-10-2004
}
// above is modified by Victor Yu. 08-15-2002

/*
 * The MOXA Smartio/Industio serial driver boot-time initialization code!
 */
#ifdef MODULE
int
init_module(void)
{
	int	ret;

	if (verbose)
		printk("Loading module mxser ...\n");
	ret = mxser_init();
	if (verbose)
		printk("Done.\n");
	return (ret);
}

void
cleanup_module(void)
{
	int i,err = 0;


	if (verbose)
		printk("Unloading module mxser ...\n");

//    if (moxaTimer_on)
//       del_timer(&moxaTimer);

	if ((err |= tty_unregister_driver(&mxvar_cdriver)))
		printk("Couldn't unregister MOXA Smartio/Industio family callout driver\n");
	if ((err |= tty_unregister_driver(&mxvar_sdriver)))
		printk("Couldn't unregister MOXA Smartio/Industio family serial driver\n");

        for(i=0; i<MXSER_BOARDS; i++){
	    struct pci_dev *pdev;

	    if(mxsercfg[i].board_type == -1)
	        continue;
            else{
		pdev = mxsercfg[i].pciInfo.pdev;
	        free_irq(mxsercfg[i].irq, &mxvar_table[i*MXSER_PORTS_PER_BOARD]);
		release_region(pci_resource_start(pdev, 2),
	       		pci_resource_len(pdev, 2));
		release_region(pci_resource_start(pdev, 3),
	       		pci_resource_len(pdev, 3));
/*
		release_region(mxsercfg[i].ioaddr[0],8*mxsercfg[i].ports);
		if ((mxsercfg[i].pciInfo.busNum == 0)&&(mxsercfg[i].pciInfo.devNum == 0))
			release_region(mxsercfg[i].vector,1);
		else
			release_region(mxsercfg[i].vector,16);
*/
            }
        }

	if (verbose)
		printk("Done.\n");

}
#endif

int mxser_initbrd(int board,struct mxser_hwconf *hwconf)
{
	struct mxser_struct *	info;
	unsigned long	flags;
                int     retval;
	int	i,n;

#if (LINUX_VERSION_CODE >= VERSION_CODE(2,4,0))
	init_MUTEX(&mxvar_tmp_buf_sem);
#endif

	n = board*MXSER_PORTS_PER_BOARD;
	info = &mxvar_table[n];
	for ( i=0; i<hwconf->ports; i++, n++, info++ ) {
		if (verbose) {
			printk("        ttyM%d/cum%d at 0x%04x ", n, n, hwconf->ioaddr[i]);
			if ( hwconf->baud_base[i] == 115200 )
		    		printk(" max. baud rate up to 115200 bps.\n");
			else
		    		printk(" max. baud rate up to 921600 bps.\n");
		}
		info->port = n;
		info->base = hwconf->ioaddr[i];
		info->irq = hwconf->irq;
		info->vector = hwconf->vector;
		info->vectormask = hwconf->vector_mask;
		info->opmode_ioaddr = hwconf->opmode_ioaddr[i];	// add by Victor Yu. 01-05-2004
		info->stop_rx = 0;
	    info->ldisc_stop_rx = 0;

		// following add by Victor Yu. 08-30-2002
		// Moxa Must UART support FIFO is 64bytes for Tx/Rx
		// but receive FIFO just can set up to 62 will be OK.
		info->IsMoxaMustChipFlag = hwconf->IsMoxaMustChipFlag;
		
		if ( (info->type == PORT_16450) || (info->type == PORT_8250) ){
			info->rx_trigger = 1;
		}else{
		
			//if ( hwconf->IsMoxaMustChipFlag )	// mask by Victor Yu. 02-10-2004
			if ( hwconf->IsMoxaMustChipFlag == MOXA_MUST_HARDWARE_ID ) {
				info->rx_trigger = 56;
			} else if ( hwconf->IsMoxaMustChipFlag == MOXA_MUST_HARDWARE_ID1 ) {
				info->rx_trigger = ID1_RX_TRIG;
			} else {
			// above add by Victor Yu. 08-30-2002
				info->rx_trigger = 14;
			}
		}
		info->baud_base = hwconf->baud_base[i];
		info->flags = ASYNC_SHARE_IRQ;
		info->type = hwconf->uart_type;
		if ( (info->type == PORT_16450) || (info->type == PORT_8250) )
		    info->xmit_fifo_size = 1;
		else{
			// following add by Victor Yu. 08-30-2002
			// if ( info->IsMoxaMustChipFlag ) { // mask by Victor Yu. 02-10-2004
			if ( info->IsMoxaMustChipFlag == MOXA_MUST_HARDWARE_ID ) {
			    info->xmit_fifo_size = 64;
			    ENABLE_MOXA_MUST_ENCHANCE_MODE(info->base);
			} else if ( info->IsMoxaMustChipFlag == MOXA_MUST_HARDWARE_ID1 ) {
			    info->xmit_fifo_size = ID1_XMIT_SIZE;
			    ENABLE_MOXA_MUST_ENCHANCE_MODE(info->base);
			}else
			    	info->xmit_fifo_size = 16;
		}
		info->MaxCanSetBaudRate = hwconf->MaxCanSetBaudRate[i];
		// above add by Victor Yu. 08-30-2002

		info->custom_divisor = hwconf->baud_base[i] * 16;
		info->close_delay = 5*HZ/10;
		info->closing_wait = 30*HZ;
		info->tqueue.routine = mxser_do_softint;
		info->tqueue.data = info;
		info->callout_termios = mxvar_cdriver.init_termios;
		info->normal_termios = mxvar_sdriver.init_termios;
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,4,0))
		init_waitqueue_head(&info->open_wait);
		init_waitqueue_head(&info->close_wait);
		init_waitqueue_head(&info->delta_msr_wait);
#endif
/* added by casper 1/11/2000 */
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
		info->icount.rx = info->icount.tx = 0;
#endif
		info->icount.cts = info->icount.dsr =
		    info->icount.dsr = info->icount.dcd = 0;
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
		info->icount.frame = info->icount.overrun =
		    info->icount.brk = info->icount.parity = 0;
#endif
/* */
                memset(&info->mon_data, 0, sizeof(struct mxser_mon));
		info->err_shadow = 0;
	}
	request_region(hwconf->ioaddr[0],8*hwconf->ports,"mxser(io)");
	if ((hwconf->pciInfo.busNum == 0)&&(hwconf->pciInfo.devNum == 0))
		request_region(hwconf->vector,1,"mxser(vector)");
	else
		request_region(hwconf->vector,16,"mxser(vector)");

	/*
	 * Allocate the IRQ if necessary
	 */
	save_flags(flags);

	/* before set INT ISR, disable all int */
	for(i=0; i<hwconf->ports; i++){
		outb(inb(hwconf->ioaddr[i] + UART_IER) & 0xf0, hwconf->ioaddr[i]+UART_IER);
	}

	n = board*MXSER_PORTS_PER_BOARD;
	info = &mxvar_table[n];
        cli();
        retval = request_irq(hwconf->irq, mxser_interrupt, IRQ_T(info),
//retval = request_irq(hwconf->irq, &mxser_interrupt, SA_SHIRQ,
				 "mxser", info);
	if ( retval ) {
	    restore_flags(flags);
	    printk("Board %d: %s", board, mxser_brdname[hwconf->board_type-1]);
	    printk("  Request irq fail,IRQ (%d) may be conflit with another device.\n",info->irq);
	    return(retval);
	}
	restore_flags(flags);

        return 0;
}


static void mxser_getcfg(int board,struct mxser_hwconf *hwconf)
{
	mxsercfg[board] = *hwconf;
}

#ifdef CONFIG_PCI
static int mxser_get_PCI_conf(int busnum,int devnum,int board_type,struct mxser_hwconf *hwconf)
{
	int		i;
//	unsigned int	val;
	unsigned int	ioaddress;
	struct pci_dev	*pdev=hwconf->pciInfo.pdev;

	hwconf->board_type = board_type;
	hwconf->ports = mxser_numports[board_type-1];
	ioaddress = pci_resource_start(pdev, 2);
	request_region(pci_resource_start(pdev, 2),
	       pci_resource_len(pdev, 2),
	       "mxser(IO)");	
/*
	pcibios_read_config_dword(busnum,devnum,PCI_BASE_ADDRESS_2,&val);
	if (val == 0xffffffff)
		return (MXSER_ERR_IOADDR);
	else
		ioaddress = val & 0xffffffc;
*/
	printk("Moxa UART base I/O address = 0x%08lX\n", (ioaddress));
	for (i = 0; i < hwconf->ports; i++) {
		hwconf->ioaddr[i] = (ioaddress) + 8*i;
		// following add by Victor Yu. 09-04-2002
		if ( board_type == MXSER_BOARD_CP104U ||
		     board_type == MXSER_BOARD_CP168U ||
		     board_type == MXSER_BOARD_CP132U ||
		     board_type == MXSER_BOARD_CP134U )
			hwconf->MaxCanSetBaudRate[i] = 230400;
		else
			hwconf->MaxCanSetBaudRate[i] = 921600;
		// above add by Victor Yu. 09-04-2002
	}

	ioaddress = pci_resource_start(pdev, 3);
	request_region(pci_resource_start(pdev, 3),
	       pci_resource_len(pdev, 3),
	       "mxser(vector)");	
/*
	pcibios_read_config_dword(busnum,devnum,PCI_BASE_ADDRESS_3,&val);
	if (val == 0xffffffff)
		return (MXSER_ERR_VECTOR);
	else
		ioaddress = val & 0xffffffc;
*/
	printk("Moxa UART vector I/O address = 0x%08lX\n", (ioaddress));
	hwconf->vector = (ioaddress);

	// following add by Victor Yu. 01-05-2004
	for (i = 0; i < hwconf->ports; i++) {
		if ( i < 4 )
			hwconf->opmode_ioaddr[i] = (ioaddress) + 4;
		else
			hwconf->opmode_ioaddr[i] = (ioaddress) + 0x0c;
	}
	outb(0, (ioaddress)+4);	// default set to RS232 mode
	outb(0, (ioaddress)+0x0c); // default set to RS232 mode
	// above add by Victor Yu. 01-05-2004

/*
	pcibios_read_config_dword(busnum,devnum,PCI_INTERRUPT_LINE,&val);
	if (val == 0xffffffff)
		return (MXSER_ERR_IRQ);
	else
		hwconf->irq = val & 0xff;
*/
	hwconf->irq = hwconf->pciInfo.pdev->irq;
//printk("Mxser IRQ = %d\n", hwconf->irq);

	// following add by Victor Yu. 08-30-2002
	hwconf->IsMoxaMustChipFlag = CheckIsMoxaMust(hwconf->ioaddr[0]);
//printk("Mxser IsMoxaMustChipFlag=%d\n", hwconf->IsMoxaMustChipFlag);
	// above add by Victor Yu. 08-30-2002

	hwconf->uart_type = PORT_16550A;
	hwconf->vector_mask = 0;
        for (i = 0; i < hwconf->ports; i++) {
		hwconf->vector_mask |= (1<<i);
		hwconf->baud_base[i] = 921600;
	}
	return(0);
}
#endif

int mxser_init(void)
{
	int			i, m, retval, b;
        int                     ret1, ret2;
#ifdef CONFIG_PCI
struct pci_dev	*pdev=NULL;
	int			n,index;
	unsigned char		busnum,devnum;
#endif
	struct mxser_hwconf	hwconf;

	for(i=0; i<MXSER_BOARDS; i++){
		mxsercfg[i].board_type = -1;
	}
	
	printk("MOXA Smartio/Industio family driver version %s\n",MXSER_VERSION);

	/* Initialize the tty_driver structure */
	memset(&mxvar_sdriver, 0, sizeof(struct tty_driver));
	mxvar_sdriver.magic = TTY_DRIVER_MAGIC;
	mxvar_sdriver.name = "ttyM";
	mxvar_sdriver.major = ttymajor;
	mxvar_sdriver.minor_start = 0;
	mxvar_sdriver.num = MXSER_PORTS + 1;
	mxvar_sdriver.type = TTY_DRIVER_TYPE_SERIAL;
	mxvar_sdriver.subtype = SERIAL_TYPE_NORMAL;
	mxvar_sdriver.init_termios = tty_std_termios;
	mxvar_sdriver.init_termios.c_cflag = B9600|CS8|CREAD|HUPCL|CLOCAL;
	mxvar_sdriver.flags = TTY_DRIVER_REAL_RAW;
	mxvar_sdriver.refcount = &mxvar_refcount;
	mxvar_sdriver.table = mxvar_tty;
	mxvar_sdriver.termios = mxvar_termios;
	mxvar_sdriver.termios_locked = mxvar_termios_locked;

	mxvar_sdriver.open = mxser_open;
	mxvar_sdriver.close = mxser_close;
	mxvar_sdriver.write = mxser_write;
	mxvar_sdriver.put_char = mxser_put_char;
	mxvar_sdriver.flush_chars = mxser_flush_chars;
	mxvar_sdriver.write_room = mxser_write_room;
	mxvar_sdriver.chars_in_buffer = mxser_chars_in_buffer;
	mxvar_sdriver.flush_buffer = mxser_flush_buffer;
	mxvar_sdriver.ioctl = mxser_ioctl;
	mxvar_sdriver.throttle = mxser_throttle;
	mxvar_sdriver.unthrottle = mxser_unthrottle;
	mxvar_sdriver.set_termios = mxser_set_termios;
	mxvar_sdriver.stop = mxser_stop;
	mxvar_sdriver.start = mxser_start;
	mxvar_sdriver.hangup = mxser_hangup;

// added by James 03-12-2004.
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,66)) /* Linux 2.1.66 */
    mxvar_sdriver.break_ctl = mxser_rs_break;
#endif
// (above) added by James.

#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
	mxvar_sdriver.wait_until_sent = mxser_wait_until_sent;
#endif

	/*
	 * The callout device is just like normal device except for
	 * major number and the subtype code.
	 */
	mxvar_cdriver = mxvar_sdriver;
	mxvar_cdriver.name = "cum";
	mxvar_cdriver.major = calloutmajor;
	mxvar_cdriver.subtype = SERIAL_TYPE_CALLOUT;

	printk("Tty devices major number = %d, callout devices major number = %d\n",ttymajor,calloutmajor);

	mxvar_diagflag = 0;
	memset(mxvar_table, 0, MXSER_PORTS * sizeof(struct mxser_struct));
	memset(&mxvar_log, 0, sizeof(struct mxser_log));

	memset(&mxser_msr, 0, sizeof(unsigned char) * (MXSER_PORTS+1));
	memset(&mon_data_ext, 0, sizeof(struct mxser_mon_ext));
	memset(&mxser_set_baud_method, 0, sizeof(int) * (MXSER_PORTS+1));

	m = 0;
	/* Start finding ISA boards here */
	for ( b=0; b<MXSER_BOARDS && m<MXSER_BOARDS; b++ ) {
            int cap;
	    if ( !(cap=mxserBoardCAP[b]) )
	        continue;

	    retval = mxser_get_ISA_conf(cap, &hwconf);

	    if ( retval != 0 )
	    	printk("Found MOXA %s board (CAP=0x%x)\n",
				mxser_brdname[hwconf.board_type-1],
				ioaddr[b]);

	    if ( retval <= 0 ) {
		if (retval == MXSER_ERR_IRQ)
			printk("Invalid interrupt number,board not configured\n");
		else if (retval == MXSER_ERR_IRQ_CONFLIT)
			printk("Invalid interrupt number,board not configured\n");
		else if (retval == MXSER_ERR_VECTOR)
			printk("Invalid interrupt vector,board not configured\n");
		else if (retval == MXSER_ERR_IOADDR)
			printk("Invalid I/O address,board not configured\n");

		continue;
	    }

	    hwconf.pciInfo.busNum = 0;
	    hwconf.pciInfo.devNum = 0;

	    mxser_getcfg(m,&hwconf);
	    
	    if(mxser_initbrd(m,&hwconf)<0)
                continue;


	    m++;
	}

	/* Start finding ISA boards from module arg */
	for ( b=0; b<MXSER_BOARDS && m<MXSER_BOARDS; b++ ) {
            int cap;
	    if ( !(cap=ioaddr[b]) )
	        continue;

	    retval = mxser_get_ISA_conf(cap, &hwconf);

	    if ( retval != 0 )
	    	printk("Found MOXA %s board (CAP=0x%x)\n",
				mxser_brdname[hwconf.board_type-1],
				ioaddr[b]);

	    if ( retval <= 0 ) {
		if (retval == MXSER_ERR_IRQ)
			printk("Invalid interrupt number,board not configured\n");
		else if (retval == MXSER_ERR_IRQ_CONFLIT)
			printk("Invalid interrupt number,board not configured\n");
		else if (retval == MXSER_ERR_VECTOR)
			printk("Invalid interrupt vector,board not configured\n");
		else if (retval == MXSER_ERR_IOADDR)
			printk("Invalid I/O address,board not configured\n");

		continue;
	    }

	    hwconf.pciInfo.busNum = 0;
	    hwconf.pciInfo.devNum = 0;

	    mxser_getcfg(m,&hwconf);
	    if(mxser_initbrd(m,&hwconf)<0)
                continue;

	    m++;
	}

	/* start finding PCI board here */
#ifdef CONFIG_PCI
#if (LINUX_VERSION_CODE < VERSION_CODE(2,1,0))
	if (pcibios_present()) {
#else
	if (pci_present()) {
#endif
		n = sizeof (mxser_pcibrds)/sizeof (mxser_pciinfo);
		index = 0;
		b = 0;
		while (b < n) {
		       pdev = pci_find_device(mxser_pcibrds[b].vendor_id,
		       mxser_pcibrds[b].device_id,
		       pdev);
			if (pcibios_find_device(mxser_pcibrds[b].vendor_id,
						mxser_pcibrds[b].device_id,
						index,
						&busnum,
						&devnum) != 0) {
				b++;
				index = 0;
				continue;
			}
			hwconf.pciInfo.busNum = busnum;
			hwconf.pciInfo.devNum = devnum;
			hwconf.pciInfo.pdev = pdev;
			printk("Found MOXA %s board(BusNo=%d,DevNo=%d)\n",mxser_brdname[mxser_pcibrds[b].board_type-1],busnum,devnum >> 3);
			index++;
			if ( m >= MXSER_BOARDS) {
				printk("Too many Smartio/Industio family boards find (maximum %d),board not configured\n",MXSER_BOARDS);
			}
			else {
				if ( pci_enable_device(pdev) ) {
					printk("Moxa SmartI/O PCI enable fail !\n");
					continue;
				}
				retval = mxser_get_PCI_conf(busnum,devnum,
					mxser_pcibrds[b].board_type,&hwconf);
				if (retval < 0) {
					if (retval == MXSER_ERR_IRQ)
						printk("Invalid interrupt number,board not configured\n");
					else if (retval == MXSER_ERR_IRQ_CONFLIT)
						printk("Invalid interrupt number,board not configured\n");
					else if (retval == MXSER_ERR_VECTOR)
						printk("Invalid interrupt vector,board not configured\n");
					else if (retval == MXSER_ERR_IOADDR)
						printk("Invalid I/O address,board not configured\n");
					continue;

				}
	    			mxser_getcfg(m,&hwconf);
	    			if(mxser_initbrd(m,&hwconf)<0)
                                    continue;
				m++;

			}

		}
	}
#endif



        ret1 = 0;
        ret2 = 0;
        if ( !(ret1=tty_register_driver(&mxvar_sdriver)) ){
            if ( !(ret2=tty_register_driver(&mxvar_cdriver)) ){
//                init_timer(&moxaTimer);
//                moxaTimer.function = mxser_poll;
//                moxaTimer.expires = jiffies + (HZ / 10);    /* 0.1 sec */
//                moxaTimer_on = 1;
//                add_timer(&moxaTimer);

                return 0;
            }else{
                tty_unregister_driver(&mxvar_sdriver);
		printk("Couldn't install MOXA Smartio/Industio family callout driver !\n");
            }
        }else
	    printk("Couldn't install MOXA Smartio/Industio family driver !\n");
printk("OK5\n");

        if(ret1 || ret2){
            for(i=0; i<MXSER_BOARDS; i++){
	        if(mxsercfg[i].board_type == -1)
	            continue;
                else{
		    free_irq(mxsercfg[i].irq, &mxvar_table[i*MXSER_PORTS_PER_BOARD]);
                }
            }
            return -1;
        }

	return(0);
}

#if 0
static void mxser_poll(unsigned long ignored)
{
    int i, status, sdsports;
    moxaTimer_on = 0;
    del_timer(&moxaTimer);
    struct mxser_struct *info;

    for(i=0; i<MXSER_BOARDS; i++) {
        if (mxsercfg[i].board_type == -1) {
            break;
        }
    }
    sdsports = i*8;
    for(i=0; i<sdsports; i++) {
        status = inb(mxvar_table[i].base + UART_MSR);
        if(status  & 0x80/*UART_MSR_DCD*/)
            GMStatus[i].dcd = 1;
        else
            GMStatus[i].dcd = 0;

        if(status  & 0x20/*UART_MSR_DSR*/)
            GMStatus[i].dsr = 1;
        else
            GMStatus[i].dsr = 0;

        if(status  & 0x10/*UART_MSR_CTS*/)
            GMStatus[i].cts = 1;
        else
            GMStatus[i].cts = 0;
	info = &(mxvar_table[i]);
	if(info->tty)
//if ( (info->tty->flip.count < TTY_FLIPBUF_SIZE/4 ) && (info->stop_rx)){
if ( (info->tty->flip.count <= LO_WATER ) && (info->stop_rx) && (info->ldisc_stop_rx==0)){
mxser_startrx(info->tty);
 info->stop_rx = 0;
}

    }

    moxaTimer.function = mxser_poll;
    moxaTimer.expires = jiffies + (HZ / 10);
    moxaTimer_on = 1;
    add_timer(&moxaTimer);
}
#endif

static void mxser_do_softint(void *private_)
{
	struct mxser_struct *	info = (struct mxser_struct *)private_;
	struct tty_struct *	tty;

	tty = info->tty;

	if (tty) {
#if (LINUX_VERSION_CODE <  VERSION_CODE(2,1,0))
	    if ( clear_bit(MXSER_EVENT_TXLOW, &info->event) ) {
	        if ( (tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
		 tty->ldisc.write_wakeup )
		(tty->ldisc.write_wakeup)(tty);
	       wake_up_interruptible(&tty->write_wait);
	    }
	    if ( clear_bit(MXSER_EVENT_HANGUP, &info->event) ) {
		tty_hangup(tty);
	    }
#else
	    if ( test_and_clear_bit(MXSER_EVENT_TXLOW, &info->event) ) {
	        if ( (tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
		 tty->ldisc.write_wakeup )
		(tty->ldisc.write_wakeup)(tty);
	        wake_up_interruptible(&tty->write_wait);
	    }
	    if ( test_and_clear_bit(MXSER_EVENT_HANGUP, &info->event) ) {
		tty_hangup(tty);
	    }
#endif
	}
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,4,0))
	MOD_DEC_USE_COUNT;
#endif
}

static unsigned char mxser_get_msr(int baseaddr, int mode, int port)
{
	unsigned char	status=0;
	unsigned long	flags;

	save_flags(flags);
	cli();
	status = inb(baseaddr + UART_MSR);
	restore_flags(flags);

	mxser_msr[port] &= 0x0F;
	mxser_msr[port] |= status;
	status = mxser_msr[port];
	if( mode )
		mxser_msr[port] = 0;
		
	return status;
}

/*
 * This routine is called whenever a serial port is opened.  It
 * enables interrupts for a serial port, linking in its async structure into
 * the IRQ chain.   It also performs the serial-specific
 * initialization for the tty structure.
 */
static int mxser_open(struct tty_struct * tty, struct file * filp)
{
	struct mxser_struct *	info;
	int			retval, line;
	unsigned long		page;
	unsigned char 		status;

	line = PORTNO(tty);
	if ( line == MXSER_PORTS )
	    return(0);
	if ( (line < 0) || (line > MXSER_PORTS) )
	    return(-ENODEV);
	info = mxvar_table + line;
	if ( !info->base )
	    return(-ENODEV);

	tty->driver_data = info;
	info->tty = tty;
	if ( !mxvar_tmp_buf ) {
	    page = get_free_page(GFP_KERNEL);
	    if ( !page )
		return(-ENOMEM);
	    if ( mxvar_tmp_buf )
		free_page(page);
	    else
		mxvar_tmp_buf = (unsigned char *)page;
	}

printk("port %d, mxser_open\r\n", info->port);
	/*
	 * Start up serial port
	 */
	retval = mxser_startup(info);
	if ( retval )
	    return(retval);

	retval = mxser_block_til_ready(tty, filp, info);
	if ( retval )
	    return(retval);

	info->count++;
	MOD_INC_USE_COUNT;

	if ( (info->count == 1) && (info->flags & ASYNC_SPLIT_TERMIOS) ) {
	    if ( tty->driver.subtype == SERIAL_TYPE_NORMAL )
		*tty->termios = info->normal_termios;
	    else
		*tty->termios = info->callout_termios;
	    mxser_change_speed(info, 0);
	}

	info->session = current->session;
	info->pgrp = current->pgrp;
	clear_bit(TTY_DONT_FLIP, &tty->flags);

	status = mxser_get_msr(info->base, 0, info->port);
	mxser_check_modem_status(info, status);

/* unmark here for very high baud rate (ex. 921600 bps) used
*/
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
	tty->low_latency = 1;
#endif
	return(0);
}

/*
 * This routine is called when the serial port gets closed.  First, we
 * wait for the last remaining data to be sent.  Then, we unlink its
 * async structure from the interrupt chain if necessary, and we free
 * that IRQ if nothing is left in the chain.
 */
static void mxser_close(struct tty_struct * tty, struct file * filp)
{
	struct mxser_struct * info = (struct mxser_struct *)tty->driver_data;
	unsigned long	flags;
	unsigned long	timeout;

	if ( PORTNO(tty) == MXSER_PORTS )
	    return;
	if ( !info )
	    return;

	save_flags(flags);
	cli();

	if ( tty_hung_up_p(filp) ) {
	    restore_flags(flags);
	    MOD_DEC_USE_COUNT;
	    return;
	}

	if ( (tty->count == 1) && (info->count != 1) ) {
	    /*
	     * Uh, oh.	tty->count is 1, which means that the tty
	     * structure will be freed.  Info->count should always
	     * be one in these conditions.  If it's greater than
	     * one, we've got real problems, since it means the
	     * serial port won't be shutdown.
	     */
	    printk("mxser_close: bad serial port count; tty->count is 1, "
		   "info->count is %d\n", info->count);
	    info->count = 1;
	}
	if ( --info->count < 0 ) {
	    printk("mxser_close: bad serial port count for ttys%d: %d\n",
		   info->port, info->count);
	    info->count = 0;
	}
	if ( info->count ) {
	    restore_flags(flags);
	    MOD_DEC_USE_COUNT;
	    return;
	}
	info->flags |= ASYNC_CLOSING;
	restore_flags(flags);	// add by Victor Yu. 09-26-2002
	/*
	 * Save the termios structure, since this port may have
	 * separate termios for callout and dialin.
	 */
	if ( info->flags & ASYNC_NORMAL_ACTIVE )
	    info->normal_termios = *tty->termios;
	if ( info->flags & ASYNC_CALLOUT_ACTIVE )
	    info->callout_termios = *tty->termios;
	/*
	 * Now we wait for the transmit buffer to clear; and we notify
	 * the line discipline to only process XON/XOFF characters.
	 */
	tty->closing = 1;
	if ( info->closing_wait != ASYNC_CLOSING_WAIT_NONE )
	    tty_wait_until_sent(tty, info->closing_wait);
	/*
	 * At this point we stop accepting input.  To do this, we
	 * disable the receive line status interrupts, and tell the
	 * interrupt driver to stop checking the data ready bit in the
	 * line status register.
	 */
	info->IER &= ~UART_IER_RLSI;
	if ( info->IsMoxaMustChipFlag )
		info->IER &= ~MOXA_MUST_RECV_ISR;
/* by William
	info->read_status_mask &= ~UART_LSR_DR;
*/
	if ( info->flags & ASYNC_INITIALIZED ) {
	    outb(info->IER, info->base + UART_IER);
	    /*
	     * Before we drop DTR, make sure the UART transmitter
	     * has completely drained; this is especially
	     * important if there is a transmit FIFO!
	     */
	    timeout = jiffies + HZ;
	    while ( !(inb(info->base + UART_LSR) & UART_LSR_TEMT) ) {
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(5);
		if ( time_after(jiffies, timeout) )
		    break;
	    }
	}
	mxser_shutdown(info);

	// following add by Victor Yu. 09-23-2002
	/*
	if ( info->IsMoxaMustChipFlag )
		SET_MOXA_MUST_NO_SOFTWARE_FLOW_CONTROL(info->base);
	*/
	// above add by Victor Yu. 09-23-2002

	if ( tty->driver.flush_buffer )
	    tty->driver.flush_buffer(tty);
	if ( tty->ldisc.flush_buffer )
	    tty->ldisc.flush_buffer(tty);
	tty->closing = 0;
	info->event = 0;
	info->tty = 0;
	if ( info->blocked_open ) {
	    if ( info->close_delay ) {
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(info->close_delay);
	    }
	    wake_up_interruptible(&info->open_wait);
	}

	info->flags &= ~(ASYNC_NORMAL_ACTIVE | ASYNC_CALLOUT_ACTIVE |
			 ASYNC_CLOSING);
	wake_up_interruptible(&info->close_wait);
//	restore_flags(flags);	// mask by Vicror Yu. 09-26-2002

	MOD_DEC_USE_COUNT;
}

static int mxser_write(struct tty_struct * tty, int from_user,
		       const unsigned char * buf, int count)
{
	int		c, total = 0;
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;
	unsigned long	flags;

	if ( !tty || !info->xmit_buf || !mxvar_tmp_buf )
	    return(0);

	if ( from_user )
	    down(&mxvar_tmp_buf_sem);
	save_flags(flags);
	cli();
	while ( 1 ) {
//	    cli();
	    c = MIN(count, MIN(SERIAL_XMIT_SIZE - info->xmit_cnt - 1,
			       SERIAL_XMIT_SIZE - info->xmit_head));
	    if ( c <= 0 )
		break;

	    if ( from_user ) {
		/*
		copy_from_user(mxvar_tmp_buf, buf, c);
		c = MIN(c, MIN(SERIAL_XMIT_SIZE - info->xmit_cnt - 1,
			       SERIAL_XMIT_SIZE - info->xmit_head));
		memcpy(info->xmit_buf + info->xmit_head, mxvar_tmp_buf, c);
		*/
		copy_from_user(info->xmit_buf+info->xmit_head, buf, c);
	    } else
		memcpy(info->xmit_buf + info->xmit_head, buf, c);
	    info->xmit_head = (info->xmit_head + c) & (SERIAL_XMIT_SIZE - 1);
	    info->xmit_cnt += c;
//	    restore_flags(flags);
	    buf += c;
	    count -= c;
	    total += c;
	}
	if ( from_user )
	    up(&mxvar_tmp_buf_sem);
	if ( info->xmit_cnt && !tty->stopped && !(info->IER & UART_IER_THRI) ) {
        	if (!tty->hw_stopped || (info->type == PORT_16550A) || (info->IsMoxaMustChipFlag)) {
	            info->IER |= UART_IER_THRI;
	            outb(info->IER, info->base + UART_IER);
	        }
	}
	restore_flags(flags);
	return(total);
}

static void mxser_put_char(struct tty_struct * tty, unsigned char ch)
{
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;
	unsigned long	flags;

	if ( !tty || !info->xmit_buf )
	    return;

	save_flags(flags);
	cli();
	if ( info->xmit_cnt >= SERIAL_XMIT_SIZE - 1 ) {
	    restore_flags(flags);
	    return;
	}

	info->xmit_buf[info->xmit_head++] = ch;
	info->xmit_head &= SERIAL_XMIT_SIZE - 1;
	info->xmit_cnt++;
	if ( !tty->stopped && !(info->IER & UART_IER_THRI) ) {
        	if (!tty->hw_stopped || (info->type == PORT_16550A) || info->IsMoxaMustChipFlag) {
	    		info->IER |= UART_IER_THRI;
	    		outb(info->IER, info->base + UART_IER);
		}
	}
	restore_flags(flags);
}

static void mxser_flush_chars(struct tty_struct * tty)
{
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;
	unsigned long	flags;

	if ( info->xmit_cnt <= 0 || tty->stopped || !info->xmit_buf ||
	     (tty->hw_stopped && (info->type!=PORT_16550A) && (!info->IsMoxaMustChipFlag)))
	    return;

	save_flags(flags);
	cli();
	info->IER |= UART_IER_THRI;
	outb(info->IER, info->base + UART_IER);
	restore_flags(flags);
}

static int mxser_write_room(struct tty_struct * tty)
{
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;
	int	ret;

	ret = SERIAL_XMIT_SIZE - info->xmit_cnt - 1;
	if ( ret < 0 )
	    ret = 0;
	return(ret);
}

static int mxser_chars_in_buffer(struct tty_struct * tty)
{
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;

	return(info->xmit_cnt);
}

static void mxser_flush_buffer(struct tty_struct * tty)
{
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;
	unsigned long flags;

	save_flags(flags);
	cli();
	info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;
/* below added by shinhay */
	outb(0x05, info->base+UART_FCR);
/* above added by shinhay */
	restore_flags(flags);
	wake_up_interruptible(&tty->write_wait);
	if ( (tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) &&
	     tty->ldisc.write_wakeup )
	    (tty->ldisc.write_wakeup)(tty);
}

static int mxser_ioctl(struct tty_struct * tty, struct file * file,
		       unsigned int cmd, unsigned long arg)
{
	int			error;
	unsigned long	flags;
	struct mxser_struct *	info = (struct mxser_struct *)tty->driver_data;
	int			retval;
	struct async_icount	cprev, cnow;	    /* kernel counter temps */
	struct serial_icounter_struct *p_cuser;     /* user space */
	unsigned long 		templ;
	if ( PORTNO(tty) == MXSER_PORTS )
	    return(mxser_ioctl_special(cmd, arg));

	// following add by Victor Yu. 01-05-2004
	if ( cmd == MOXA_SET_OP_MODE || cmd == MOXA_GET_OP_MODE ) {
		int	opmode, p;
		static unsigned char ModeMask[]={0xfc, 0xf3, 0xcf, 0x3f};
		int             shiftbit;
		unsigned char	val, mask;

		p = info->port % 4;
		if ( cmd == MOXA_SET_OP_MODE ) {
	    		error = verify_area(VERIFY_READ, (void *)arg, sizeof(int));
	    		if ( error )
				return(error);
	    		get_from_user(opmode,(int *)arg);
			if ( opmode != RS232_MODE && opmode != RS485_2WIRE_MODE && opmode != RS422_MODE && opmode != RS485_4WIRE_MODE )
				return -EFAULT;
			mask = ModeMask[p];
			shiftbit = p * 2;
			val = inb(info->opmode_ioaddr);
			val &= mask;
			val |= (opmode << shiftbit);
			outb(val, info->opmode_ioaddr);
		} else {
	    		error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(int));
	    		if ( error )
				return(error);
			shiftbit = p * 2;
			opmode = inb(info->opmode_ioaddr) >> shiftbit;
			opmode &= OP_MODE_MASK;
	    		copy_to_user((int*)arg, &opmode, sizeof(int));
		}
		return 0;
	}
	// above add by Victor Yu. 01-05-2004

	if ( (cmd != TIOCGSERIAL) && (cmd != TIOCMIWAIT) &&
	     (cmd != TIOCGICOUNT) ) {
	    if ( tty->flags & (1 << TTY_IO_ERROR) )
		return(-EIO);
	}
	switch ( cmd ) {
	case TCSBRK:	/* SVID version: non-zero arg --> no break */
	    retval = tty_check_change(tty);
	    if ( retval )
		return(retval);
	    tty_wait_until_sent(tty, 0);
	    if ( !arg )
		mxser_send_break(info, HZ/4);		/* 1/4 second */
	    return(0);
	case TCSBRKP:	/* support for POSIX tcsendbreak() */
	    retval = tty_check_change(tty);
	    if ( retval )
		return(retval);
	    tty_wait_until_sent(tty, 0);
	    mxser_send_break(info, arg ? arg*(HZ/10) : HZ/4);
	    return(0);
	case TIOCGSOFTCAR:
	    error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(long));
	    if ( error )
		return(error);
	    put_to_user(C_CLOCAL(tty) ? 1 : 0, (unsigned long *)arg);
	    return 0;
	case TIOCSSOFTCAR:
	    error = verify_area(VERIFY_READ, (void *)arg, sizeof(long));
	    if ( error )
		return(error);
	    get_from_user(templ,(unsigned long *)arg);
	    arg = templ;
	    tty->termios->c_cflag = ((tty->termios->c_cflag & ~CLOCAL) |
				    (arg ? CLOCAL : 0));
	    return(0);
	case TIOCMGET:
	    error = verify_area(VERIFY_WRITE, (void *)arg,
				sizeof(unsigned int));
	    if ( error )
		return(error);
	    return(mxser_get_modem_info(info, (unsigned int *)arg));
	case TIOCMBIS:
	case TIOCMBIC:
	case TIOCMSET:
	    return(mxser_set_modem_info(info, cmd, (unsigned int *)arg));
	case TIOCGSERIAL:
	    error = verify_area(VERIFY_WRITE, (void *)arg,
				sizeof(struct serial_struct));
	    if ( error )
		return(error);
	    return(mxser_get_serial_info(info, (struct serial_struct *)arg));
	case TIOCSSERIAL:
	    error = verify_area(VERIFY_READ, (void *)arg,
				sizeof(struct serial_struct));
	    if ( error )
		return(error);
	    return(mxser_set_serial_info(info, (struct serial_struct *)arg));
	case TIOCSERGETLSR: /* Get line status register */
	    error = verify_area(VERIFY_WRITE, (void *)arg,
				sizeof(unsigned int));
	    if ( error )
		return(error);
	    else
		return(mxser_get_lsr_info(info, (unsigned int *)arg));
	/*
	 * Wait for any of the 4 modem inputs (DCD,RI,DSR,CTS) to change
	 * - mask passed in arg for lines of interest
	 *   (use |'ed TIOCM_RNG/DSR/CD/CTS for masking)
	 * Caller should use TIOCGICOUNT to see which one it was
	 */
	case TIOCMIWAIT:
	    save_flags(flags);
	    cli();
	    cprev = info->icount;   /* note the counters on entry */
	    restore_flags(flags);
	    while ( 1 ) {
		interruptible_sleep_on(&info->delta_msr_wait);
		/* see if a signal did it */
		if ( signal_pending(current) )
		    return(-ERESTARTSYS);
	        save_flags(flags);
		cli();
		cnow = info->icount;	/* atomic copy */
	        restore_flags(flags);
		if ( cnow.rng == cprev.rng && cnow.dsr == cprev.dsr &&
		     cnow.dcd == cprev.dcd && cnow.cts == cprev.cts )
		    return(-EIO);	/* no change => error */
		if ( ((arg & TIOCM_RNG) && (cnow.rng != cprev.rng)) ||
		     ((arg & TIOCM_DSR) && (cnow.dsr != cprev.dsr)) ||
		     ((arg & TIOCM_CD)	&& (cnow.dcd != cprev.dcd)) ||
		     ((arg & TIOCM_CTS) && (cnow.cts != cprev.cts)) ) {
		    return(0);
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
	    error = verify_area(VERIFY_WRITE, (void *)arg,
				sizeof(struct serial_icounter_struct));
	    if ( error )
		return(error);
	    save_flags(flags);
	    cli();
	    cnow = info->icount;
	    restore_flags(flags);
	    p_cuser = (struct serial_icounter_struct *)arg;
/* modified by casper 1/11/2000 */
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
	        if (put_user(cnow.frame, &p_cuser->frame))
			return -EFAULT;
	        if (put_user(cnow.brk, &p_cuser->brk))
			return -EFAULT;
	        if (put_user(cnow.overrun, &p_cuser->overrun))
			return -EFAULT;
	        if (put_user(cnow.buf_overrun, &p_cuser->buf_overrun))
			return -EFAULT;
	        if (put_user(cnow.parity, &p_cuser->parity))
			return -EFAULT;
	        if (put_user(cnow.rx, &p_cuser->rx))
			return -EFAULT;
	        if (put_user(cnow.tx, &p_cuser->tx))
			return -EFAULT;
#endif

	    put_to_user(cnow.cts, &p_cuser->cts);
	    put_to_user(cnow.dsr, &p_cuser->dsr);
	    put_to_user(cnow.rng, &p_cuser->rng);
	    put_to_user(cnow.dcd, &p_cuser->dcd);

/* */
	    return(0);
	case MOXA_HighSpeedOn:
	    error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(int));
	    if ( error )
		return(error);
	    put_to_user(info->baud_base != 115200 ? 1 : 0, (int *)arg);
	    return(0);

	case MOXA_SDS_RSTICOUNTER: {
	    info->mon_data.rxcnt = 0;
	    info->mon_data.txcnt = 0;
	    return(0);
    }
// (above) added by James.
    case MOXA_ASPP_SETBAUD:{
        long    baud;

		error = verify_area(VERIFY_READ, (void *)arg, sizeof(long));
		if ( error )
    		return(error);
		get_from_user(baud, (long *)arg);

        mxser_set_baud(info, baud);
        return (0);
        }
    case MOXA_ASPP_GETBAUD:
		error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(long));
		if ( error )
    		return(error);

	    if (copy_to_user((struct mxser_log *)arg, &info->realbaud, sizeof(long)))
	        return -EFAULT;

        return (0);

    case MOXA_ASPP_OQUEUE:{
        int len, lsr;

		error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(int));
		if ( error )
    		return(error);
        len = mxser_chars_in_buffer(tty);

        lsr = inb(info->base+ UART_LSR) & UART_LSR_TEMT;

        len += (lsr ? 0: 1);

		if (copy_to_user((struct mxser_log *)arg, &len, sizeof(int)))
	        return -EFAULT;

        return (0);
    }
    case MOXA_ASPP_MON:{
        int mcr, status;
		error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(struct mxser_mon));
		if ( error )
    		return(error);
//    	info->mon_data.ser_param = tty->termios->c_cflag;

		status = mxser_get_msr(info->base, 1, info->port);
		mxser_check_modem_status(info, status);

		mcr = inb(info->base + UART_MCR);
		if(mcr & MOXA_MUST_MCR_XON_FLAG)
	        info->mon_data.hold_reason &= ~NPPI_NOTIFY_XOFFHOLD;
		else
	        info->mon_data.hold_reason |= NPPI_NOTIFY_XOFFHOLD;
	
		if(mcr & MOXA_MUST_MCR_TX_XON)
	        info->mon_data.hold_reason &= ~NPPI_NOTIFY_XOFFXENT;
		else
	        info->mon_data.hold_reason |= NPPI_NOTIFY_XOFFXENT;
    	
    	if(info->tty->hw_stopped)
    	    info->mon_data.hold_reason |= NPPI_NOTIFY_CTSHOLD;
        else
    	    info->mon_data.hold_reason &= ~NPPI_NOTIFY_CTSHOLD;
    	
   	
		if (copy_to_user((struct mxser_mon *)arg, &(info->mon_data), sizeof(struct mxser_mon)))
	        return -EFAULT;

        return (0);
        
        }

    case MOXA_ASPP_LSTATUS:{
	error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(struct mxser_mon));
	if ( error )
    		return(error);
    		
	if (copy_to_user((struct mxser_mon *)arg, &(info->err_shadow), 
		sizeof(unsigned char)))
	        return -EFAULT;

        info->err_shadow = 0;
        return (0);
        
        }
	case MOXA_SET_BAUD_METHOD:{
		int method;
		error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(int));
		if ( error )
    		return(error);
   	    get_from_user(method,(int *)arg);
		mxser_set_baud_method[info->port] = method;
		if (copy_to_user((struct mxser_log *)arg, &method, sizeof(int)))
	        return -EFAULT;

        return (0);
    }
	default:
	    return(-ENOIOCTLCMD);
	}
	return(0);
}

static int mxser_ioctl_special(unsigned int cmd, unsigned long arg)
{
	int		error, i, result, status;

	switch ( cmd ) {
	case MOXA_GET_CONF:
	    error = verify_area(VERIFY_WRITE, (void *)arg,
			    sizeof(struct mxser_hwconf)*4);
	    if ( error )
		return(error);
	    copy_to_user((struct mxser_hwconf *)arg, mxsercfg,
			    sizeof(struct mxser_hwconf)*4);
	    return 0;
        case MOXA_GET_MAJOR:
	    error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(int));
	    if ( error )
                return(error);
	    copy_to_user((int*)arg, &ttymajor, sizeof(int));
            return 0;

        case MOXA_GET_CUMAJOR:
	    error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(int));
	    if ( error )
                return(error);
	    copy_to_user((int*)arg, &calloutmajor, sizeof(int));
            return 0;

	case MOXA_CHKPORTENABLE:
	    error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(long));
	    if ( error )
		return(error);
	    result = 0;
	    for ( i=0; i<MXSER_PORTS; i++ ) {
		if ( mxvar_table[i].base )
		    result |= (1 << i);
	    }
	    put_to_user(result, (unsigned long *)arg);
	    return(0);
	case MOXA_GETDATACOUNT:
	    error = verify_area(VERIFY_WRITE, (void *)arg,
				sizeof(struct mxser_log));
	    if ( error )
		return(error);
	    copy_to_user((struct mxser_log *)arg, &mxvar_log, sizeof(mxvar_log));
	    return(0);
        case MOXA_GETMSTATUS:
	    error = verify_area(VERIFY_WRITE, (void *)arg,
				sizeof(struct mxser_mstatus) * MXSER_PORTS);
	    if ( error )
		return(error);

        for(i=0; i<MXSER_PORTS; i++){
            GMStatus[i].ri = 0;
		if ( !mxvar_table[i].base ){
                    GMStatus[i].dcd = 0;
                    GMStatus[i].dsr = 0;
                    GMStatus[i].cts = 0;
		    continue;
                }

	        if ( !mxvar_table[i].tty || !mxvar_table[i].tty->termios )
                    GMStatus[i].cflag=mxvar_table[i].normal_termios.c_cflag;
                else
                    GMStatus[i].cflag = mxvar_table[i].tty->termios->c_cflag;

                status = inb(mxvar_table[i].base + UART_MSR);
                if(status  & 0x80/*UART_MSR_DCD*/)
                    GMStatus[i].dcd = 1;
                else
                    GMStatus[i].dcd = 0;

                if(status  & 0x20/*UART_MSR_DSR*/)
                    GMStatus[i].dsr = 1;
                else
                    GMStatus[i].dsr = 0;


                if(status  & 0x10/*UART_MSR_CTS*/)
                    GMStatus[i].cts = 1;
                else
                    GMStatus[i].cts = 0;
            }
            copy_to_user((struct mxser_mstatus *)arg, GMStatus,
                                    sizeof(struct mxser_mstatus) * MXSER_PORTS);
            return 0;
    case MOXA_ASPP_MON_EXT:{
        int status;
		error = verify_area(VERIFY_WRITE, (void *)arg, sizeof(struct mxser_mon_ext));
		if ( error )
    		return(error);

		for(i=0; i<MXSER_PORTS; i++){

			if ( !mxvar_table[i].base )
				continue;
			status = mxser_get_msr(mxvar_table[i].base, 0, i);
//			mxser_check_modem_status(&mxvar_table[i], status);
			if ( status & UART_MSR_TERI )	    mxvar_table[i].icount.rng++;
			if ( status & UART_MSR_DDSR )	    mxvar_table[i].icount.dsr++;
			if ( status & UART_MSR_DDCD )	    mxvar_table[i].icount.dcd++;
			if ( status & UART_MSR_DCTS )	    mxvar_table[i].icount.cts++;
			mxvar_table[i].mon_data.modem_status = status;
			mon_data_ext.rx_cnt[i] = mxvar_table[i].mon_data.rxcnt;
			mon_data_ext.tx_cnt[i] = mxvar_table[i].mon_data.txcnt;
			mon_data_ext.up_rxcnt[i] = mxvar_table[i].mon_data.up_rxcnt;
			mon_data_ext.up_txcnt[i] = mxvar_table[i].mon_data.up_txcnt;
			mon_data_ext.modem_status[i] = mxvar_table[i].mon_data.modem_status;
		}
		if (copy_to_user((struct mxser_mon_ext *)arg, &mon_data_ext, sizeof(struct mxser_mon_ext)))
	        return -EFAULT;

        return (0);
        
        }
	default:
	    return(-ENOIOCTLCMD);
	}
	return(0);
}


static void mxser_stoprx(struct tty_struct * tty)
{
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;
	unsigned long	flags;

	if ( I_IXOFF(tty) ) {
	    save_flags(flags);
	    cli();

	    // following add by Victor Yu. 09-02-2002
	    if ( info->IsMoxaMustChipFlag ) {
		info->IER &= ~MOXA_MUST_RECV_ISR;
		outb(info->IER, info->base+UART_IER);
	    } else {
	    // above add by Victor Yu. 09-02-2002

	    info->x_char = STOP_CHAR(tty);
	//  outb(info->IER, 0); // mask by Victor Yu. 09-02-2002
	    outb(0, info->base+UART_IER);
	    info->IER |= UART_IER_THRI;
	    outb(info->IER, info->base + UART_IER);	/* force Tx interrupt */
	    }	// add by Victor Yu. 09-02-2002
	    restore_flags(flags);
	}

	if ( info->tty->termios->c_cflag & CRTSCTS ) {
	    info->MCR &= ~UART_MCR_RTS;
	    save_flags(flags);
	    cli();
	    outb(info->MCR, info->base + UART_MCR);
	    restore_flags(flags);
	}
}

static void mxser_startrx(struct tty_struct * tty)
{
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;
	unsigned long	flags;

        if ( I_IXOFF(tty) ) {
	    if ( info->x_char )
		info->x_char = 0;
	    else {
		save_flags(flags);
		cli();

		// following add by Victor Yu. 09-02-2002
		if ( info->IsMoxaMustChipFlag ) {
			info->IER |= MOXA_MUST_RECV_ISR;
			outb(info->IER, info->base+UART_IER);
		} else {
		// above add by Victor Yu. 09-02-2002

		info->x_char = START_CHAR(tty);
	//	outb(info->IER, 0);	// mask by Victor Yu. 09-02-2002
		outb(0, info->base+UART_IER);	// add by Victor Yu. 09-02-2002
		info->IER |= UART_IER_THRI;		/* force Tx interrupt */
		outb(info->IER, info->base + UART_IER);
		}	// add by Victor Yu. 09-02-2002
		restore_flags(flags);
	    }
	}

	if ( info->tty->termios->c_cflag & CRTSCTS ) {
	    info->MCR |= UART_MCR_RTS;
	    save_flags(flags);
	    cli();
	    outb(info->MCR, info->base + UART_MCR);
	    restore_flags(flags);
	}
}

/*
 * This routine is called by the upper-layer tty layer to signal that
 * incoming characters should be throttled.
 */
static void mxser_throttle(struct tty_struct * tty)
{
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;
        info->ldisc_stop_rx = 1;
        mxser_stoprx(tty);
}

static void mxser_unthrottle(struct tty_struct * tty)
{
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;
       info->ldisc_stop_rx = 0;
        mxser_startrx(tty);
}

static void mxser_set_termios(struct tty_struct * tty,
			      struct termios * old_termios)
{
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;
	unsigned long	flags;

	if ( (tty->termios->c_cflag != old_termios->c_cflag) ||
	     (RELEVANT_IFLAG(tty->termios->c_iflag) !=
	      RELEVANT_IFLAG(old_termios->c_iflag)) ) {

		mxser_change_speed(info, old_termios);

		if ( (old_termios->c_cflag & CRTSCTS) &&
	     	     !(tty->termios->c_cflag & CRTSCTS) ) {
	    		tty->hw_stopped = 0;
	    		mxser_start(tty);
		}
	}

/* Handle sw stopped */
	if ( (old_termios->c_iflag & IXON) &&
     	     !(tty->termios->c_iflag & IXON) ) {
    		tty->stopped = 0;

		// following add by Victor Yu. 09-02-2002
		if ( info->IsMoxaMustChipFlag ) {
			save_flags(flags);
			cli();
			DISABLE_MOXA_MUST_RX_SOFTWARE_FLOW_CONTROL(info->base);
			restore_flags(flags);
		}
		// above add by Victor Yu. 09-02-2002

    		mxser_start(tty);
	}
}

/*
 * mxser_stop() and mxser_start()
 *
 * This routines are called before setting or resetting tty->stopped.
 * They enable or disable transmitter interrupts, as necessary.
 */
static void mxser_stop(struct tty_struct * tty)
{
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;
	unsigned long	flags;

	save_flags(flags);
	cli();
	if ( info->IER & UART_IER_THRI ) {
	    info->IER &= ~UART_IER_THRI;
	    outb(info->IER, info->base + UART_IER);
	}
	restore_flags(flags);
}

static void mxser_start(struct tty_struct * tty)
{
	struct mxser_struct *info = (struct mxser_struct *)tty->driver_data;
	unsigned long	flags;

	save_flags(flags);
	cli();
	if ( info->xmit_cnt && info->xmit_buf &&
	     !(info->IER & UART_IER_THRI) ) {
	    info->IER |= UART_IER_THRI;
	    outb(info->IER, info->base + UART_IER);
	}
	restore_flags(flags);
}

#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
/*
 * mxser_wait_until_sent() --- wait until the transmitter is empty
 */
static void mxser_wait_until_sent(struct tty_struct *tty, int timeout)
{
	struct mxser_struct * info = (struct mxser_struct *)tty->driver_data;
	unsigned long orig_jiffies, char_time;
	int lsr;

	if (info->type == PORT_UNKNOWN)
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
	while (!((lsr = inb(info->base+ UART_LSR)) & UART_LSR_TEMT)) {
#ifdef SERIAL_DEBUG_RS_WAIT_UNTIL_SENT
		printk("lsr = %d (jiff=%lu)...", lsr, jiffies);
#endif
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,3,0))
		set_current_state(TASK_INTERRUPTIBLE);
#else
		current->state = TASK_INTERRUPTIBLE;
#endif
		schedule_timeout(char_time);
		if (signal_pending(current))
			break;
		if (timeout && time_after(jiffies, orig_jiffies + timeout))
			break;
	}
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,3,0))
		set_current_state(TASK_RUNNING);
#else
		current->state = TASK_RUNNING;
#endif

#ifdef SERIAL_DEBUG_RS_WAIT_UNTIL_SENT
	printk("lsr = %d (jiff=%lu)...done\n", lsr, jiffies);
#endif
}
#endif


/*
 * This routine is called by tty_hangup() when a hangup is signaled.
 */
void mxser_hangup(struct tty_struct * tty)
{
	struct mxser_struct * info = (struct mxser_struct *)tty->driver_data;

	mxser_flush_buffer(tty);
	mxser_shutdown(info);
	info->event = 0;
	info->count = 0;
	info->flags &= ~(ASYNC_NORMAL_ACTIVE|ASYNC_CALLOUT_ACTIVE);
	info->tty = 0;
	wake_up_interruptible(&info->open_wait);
}


// added by James 03-12-2004.
/*
 * mxser_rs_break() --- routine which turns the break handling on or off
 */
#if (LINUX_VERSION_CODE >= 131394) /* Linux 2.1.66 */
static void mxser_rs_break(struct tty_struct *tty, int break_state)
{
	struct mxser_struct * info = (struct mxser_struct *)tty->driver_data;
	unsigned long flags;

	save_flags(flags);
	cli();
	if (break_state == -1)
		outb(inb(info->base + UART_LCR) | UART_LCR_SBC, info->base + UART_LCR);
	else
		outb(inb(info->base + UART_LCR) & ~UART_LCR_SBC, info->base + UART_LCR);
	restore_flags(flags);
}
#endif
// (above) added by James.


/*
 * This is the serial driver's generic interrupt routine
 */
static void mxser_interrupt(int irq, void *dev_id, struct pt_regs * regs)
{
	int			status, iir, i;
	struct mxser_struct *	info;
	struct mxser_struct *	port;
	int			max, irqbits, bits, msr;
	int			pass_counter = 0;

        port = 0;
        for(i=0; i<MXSER_BOARDS; i++){
            if(dev_id == &(mxvar_table[i*MXSER_PORTS_PER_BOARD])){
                port = dev_id;
                break;
            }
        }

        if(i==MXSER_BOARDS){
            return;
	}
        if(port==0){
            return;
	}
        max = mxser_numports[mxsercfg[i].board_type-1];
	while ( 1 ) {
	    irqbits = inb(port->vector) & port->vectormask;
	    if ( irqbits == port->vectormask ){
			break;
		}
	    for ( i=0, bits=1; i<max; i++, irqbits |= bits, bits <<= 1 ) {
			if ( irqbits == port->vectormask ){
		    	break;
			}
			if ( bits & irqbits )
			    continue;
			info = port + i;

			// following add by Victor Yu. 09-13-2002
			iir = inb(info->base+UART_IIR);
			if ( iir & UART_IIR_NO_INT )
				continue;
			iir &= MOXA_MUST_IIR_MASK;
			if ( !info->tty ) {
				status = inb(info->base+UART_LSR);
				outb(0x27, info->base+UART_FCR);
				inb(info->base+UART_MSR);
				continue;
			}
		// above add by Victor Yu. 09-13-2002
	/*
if ( info->tty->flip.count < TTY_FLIPBUF_SIZE/4 ){
 info->IER |= MOXA_MUST_RECV_ISR;
 outb(info->IER, info->base + UART_IER);
}
*/


		/* mask by Victor Yu. 09-13-2002
		if ( !info->tty ||
		     (inb(info->base + UART_IIR) & UART_IIR_NO_INT) )
		    continue;
		*/
		/* mask by Victor Yu. 09-02-2002
		status = inb(info->base + UART_LSR) & info->read_status_mask;
		*/

		// following add by Victor Yu. 09-02-2002
			status = inb(info->base+UART_LSR);
		
			if(status & UART_LSR_PE)
			{
			        info->err_shadow |= NPPI_NOTIFY_PARITY;
	printk("UART Parity Error\r\n");
			}
			if(status & UART_LSR_FE)
			{
			        info->err_shadow |= NPPI_NOTIFY_FRAMING;
	printk("UART Framing Error\r\n");		        
			}
			if(status & UART_LSR_OE) 
			{
			        info->err_shadow |= NPPI_NOTIFY_HW_OVERRUN;
	printk("UART Overrun Error\r\n");
			}
			if(status & UART_LSR_BI)
			        info->err_shadow |= NPPI_NOTIFY_BREAK;
			        
			if ( info->IsMoxaMustChipFlag ) {
				/*
				if ( (status & 0x02) && !(status & 0x01) ) {
					outb(info->base+UART_FCR,  0x23);
					continue;
				}
				*/
				if ( iir == MOXA_MUST_IIR_GDA ||
				     iir == MOXA_MUST_IIR_RDA ||
				     iir == MOXA_MUST_IIR_RTO ||
				     iir == MOXA_MUST_IIR_LSR )
					mxser_receive_chars(info, &status);
				
			} else {
			// above add by Victor Yu. 09-02-2002

				status &= info->read_status_mask;
				if ( status & UART_LSR_DR )
				    mxser_receive_chars(info, &status);
			}
			msr = inb(info->base + UART_MSR);
			if ( msr & UART_MSR_ANY_DELTA ) {
			    mxser_check_modem_status(info, msr);
			}

			// following add by Victor Yu. 09-13-2002
			if ( info->IsMoxaMustChipFlag ) {
				if ((iir == 0x02 ) && (status & UART_LSR_THRE)){
					mxser_transmit_chars(info);
				}
			} else {
			// above add by Victor Yu. 09-13-2002

				if ( status & UART_LSR_THRE ) {
	/* 8-2-99 by William
				    if ( info->x_char || (info->xmit_cnt > 0) )
	*/
					mxser_transmit_chars(info);
				}
			}
		    }
		    if ( pass_counter++ > MXSER_ISR_PASS_LIMIT ) {
#if 0
			printk("MOXA Smartio/Industio family driver interrupt loop break\n");
#endif
			break;	/* Prevent infinite loops */
	    }
	}
}

static inline void mxser_receive_chars(struct mxser_struct *info,
					 int *status)
{
	struct tty_struct *	tty = info->tty;
	unsigned char		ch, gdl;
	int			ignored = 0;
	int			cnt = 0;
	unsigned char           *cp;
	char                    *fp;
	int			count;
	int 			recv_room;

#if 0
        cp = tty->flip.char_buf + tty->flip.count;
        fp = tty->flip.flag_buf + tty->flip.count;
        count = tty->flip.count;
#else
	recv_room = tty->ldisc.receive_room(tty);
	/*if((recv_room == 0) && (!info->ldisc_stop_rx)){
		mxser_throttle(tty);
		return;
	}*/	
		
        cp = tty->flip.char_buf;
        fp = tty->flip.flag_buf;
        count = 0;
#endif

	// following add by Victor Yu. 09-02-2002
	//if ( info->IsMoxaMustChipFlag ) {

		if ( *status & UART_LSR_SPECIAL ) {
			goto intr_old;
		}

		// following add by Victor Yu. 02-11-2004
		/*if ( info->IsMoxaMustChipFlag == MOXA_MUST_HARDWARE_ID1 &&
		     (*status & MOXA_MUST_LSR_RERR) )
			goto intr_old;*/
		// above add by Victor Yu. 02-14-2004
		if(*status & MOXA_MUST_LSR_RERR)
			goto intr_old;

		gdl = inb(info->base+MOXA_MUST_GDL_REGISTER);
//printk("Mxser:jiffies=%d, gdl=%d\n", jiffies, gdl);

		//if ( info->IsMoxaMustChipFlag == MOXA_MUST_HARDWARE_ID ) // add by Victor Yu. 02-11-2004
		gdl &= MOXA_MUST_GDL_MASK;
		if(gdl >= recv_room) {
			if(!info->ldisc_stop_rx)
				mxser_throttle(tty);
			return;
		}
		while ( gdl-- ) {
	    		ch = inb(info->base + UART_RX);
	    		count++;
			*cp++ = ch;
		    	*fp++ = 0;
			cnt++;
			/*
                        if((count>=HI_WATER) && (info->stop_rx==0)){
                                mxser_stoprx(tty);
                                info->stop_rx=1;
                                break;
                        }*/
		}
		goto end_intr;
	//}
intr_old:
	// above add by Victor Yu. 09-02-2002

	do {
		/*
            if((count>=HI_WATER) && (info->stop_rx==0)){
                mxser_stoprx(tty);
                info->stop_rx=1;
                break;
            }
	    */

	    ch = inb(info->base + UART_RX);
	    // following add by Victor Yu. 09-02-2002
	    if ( info->IsMoxaMustChipFlag && (*status&UART_LSR_OE) /*&& !(*status&UART_LSR_DR)*/ )
			outb(0x23, info->base+UART_FCR);
	    	*status &= info->read_status_mask;
	    // above add by Victor Yu. 09-02-2002
	    	if ( *status & info->ignore_status_mask ) {
			if ( ++ignored > 100 )
		    	break;
	    } else {
		count++;
		if ( *status & UART_LSR_SPECIAL ) {
		    if ( *status & UART_LSR_BI ) {
			*fp++ = TTY_BREAK;
/* added by casper 1/11/2000 */
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
			info->icount.brk++;
#endif

/* */
			if ( info->flags & ASYNC_SAK )
			    do_SAK(tty);
		    } else if ( *status & UART_LSR_PE ) {
			*fp++ = TTY_PARITY;
/* added by casper 1/11/2000 */
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
			info->icount.parity++;
#endif
/* */
		    } else if ( *status & UART_LSR_FE ) {
			*fp++ = TTY_FRAME;
/* added by casper 1/11/2000 */
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
			info->icount.frame++;
#endif
/* */
		    } else if ( *status & UART_LSR_OE ) {
			*fp++ = TTY_OVERRUN;
/* added by casper 1/11/2000 */
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
			info->icount.overrun++;
#endif
/* */
		    } else
			*fp++ = 0;
		} else
		    *fp++ = 0;
		*cp++ = ch;
		cnt++;
		if(cnt>=recv_room){
			if(!info->ldisc_stop_rx)
				mxser_throttle(tty);
			break;
		}

	    }

	    // following add by Victor Yu. 09-02-2002
	    if ( info->IsMoxaMustChipFlag )
		break;
	    // above add by Victor Yu. 09-02-2002

	    /* mask by Victor Yu. 09-02-2002
	    *status = inb(info->base + UART_LSR) & info->read_status_mask;
	    */
	    // following add by Victor Yu. 09-02-2002
	    *status = inb(info->base+UART_LSR);
	    // above add by Victor Yu. 09-02-2002
	} while ( *status & UART_LSR_DR );

end_intr:	// add by Victor Yu. 09-02-2002

	mxvar_log.rxcnt[info->port] += cnt;
	info->mon_data.rxcnt += cnt;
	info->mon_data.up_rxcnt += cnt;

	tty->ldisc.receive_buf(tty, tty->flip.char_buf, tty->flip.flag_buf, count);

		/*
	tty->flip.count = count;
        if(info->ldisc_stop_rx==0){
                tty->flip.count = 0;
	        tty->ldisc.receive_buf(tty, tty->flip.char_buf,
			       tty->flip.flag_buf, count);
        }
	*/


}

static inline void mxser_transmit_chars(struct mxser_struct *info)
{
	int	count, cnt;

	if ( info->x_char ) {
	    outb(info->x_char, info->base + UART_TX);
	    info->x_char = 0;
	    mxvar_log.txcnt[info->port]++;
	    info->mon_data.txcnt++;
	    info->mon_data.up_txcnt++;

/* added by casper 1/11/2000 */
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
	    info->icount.tx++;
#endif
/* */
	    return;
	}

	if ( info->xmit_buf == 0 )
		return;

	if ((info->xmit_cnt <= 0) || info->tty->stopped ||
	    (info->tty->hw_stopped && (info->type != PORT_16550A) &&(!info->IsMoxaMustChipFlag))) {
		info->IER &= ~UART_IER_THRI;
		outb(info->IER, info->base + UART_IER);
		return;
	}

	cnt = info->xmit_cnt;
	count = info->xmit_fifo_size;
	do {
        // printk("info->xmit_buf[info->xmit_tail] = %Xh\r\n", info->xmit_buf[info->xmit_tail]);
	    outb(info->xmit_buf[info->xmit_tail++], info->base + UART_TX);
	    info->xmit_tail = info->xmit_tail & (SERIAL_XMIT_SIZE - 1);
	    if ( --info->xmit_cnt <= 0 )
		break;
	} while ( --count > 0 );
	mxvar_log.txcnt[info->port] += (cnt - info->xmit_cnt);

// added by James 03-12-2004.
	info->mon_data.txcnt += (cnt - info->xmit_cnt);
	info->mon_data.up_txcnt += (cnt - info->xmit_cnt);
// (above) added by James.

/* added by casper 1/11/2000 */
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
        info->icount.tx += (cnt - info->xmit_cnt);
#endif
/* */

	if ( info->xmit_cnt < WAKEUP_CHARS ) {
		set_bit(MXSER_EVENT_TXLOW,&info->event);
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,4,0))
		MOD_INC_USE_COUNT;
		if (schedule_task(&info->tqueue) == 0)
			MOD_DEC_USE_COUNT;
#else

		queue_task(&info->tqueue,&tq_scheduler);
#endif
	}
	if (info->xmit_cnt <= 0) {
		info->IER &= ~UART_IER_THRI;
		outb(info->IER, info->base + UART_IER);
	}
}

static inline void mxser_check_modem_status(struct mxser_struct *info,
					      int status)
{
	/* update input line counters */
	if ( status & UART_MSR_TERI )
	    info->icount.rng++;
	if ( status & UART_MSR_DDSR )
	    info->icount.dsr++;
	if ( status & UART_MSR_DDCD )
	    info->icount.dcd++;
	if ( status & UART_MSR_DCTS )
	    info->icount.cts++;
	info->mon_data.modem_status = status;
	wake_up_interruptible(&info->delta_msr_wait);
	

	if ( (info->flags & ASYNC_CHECK_CD) && (status & UART_MSR_DDCD) ) {
	    if ( status & UART_MSR_DCD )
		wake_up_interruptible(&info->open_wait);
	    else if ( !((info->flags & ASYNC_CALLOUT_ACTIVE) &&
		      (info->flags & ASYNC_CALLOUT_NOHUP)) )

	        set_bit(MXSER_EVENT_HANGUP,&info->event);
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,4,0))
	        MOD_INC_USE_COUNT;
		if (schedule_task(&info->tqueue) == 0)
	       		MOD_DEC_USE_COUNT;
# else
# if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
		queue_task(&info->tqueue, &tq_scheduler);
# else
		queue_task_irq_off(&info->tqueue, &tq_scheduler);
# endif
#endif

	}

	if ( info->flags & ASYNC_CTS_FLOW ) {
	    if ( info->tty->hw_stopped ) {
			if (status & UART_MSR_CTS ){
		    	info->tty->hw_stopped = 0;

		    	if ((info->type != PORT_16550A) && (!info->IsMoxaMustChipFlag)){
					info->IER |= UART_IER_THRI;
					outb(info->IER, info->base + UART_IER);
		    	}
	       		set_bit(MXSER_EVENT_TXLOW,&info->event);
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,4,0))
				MOD_INC_USE_COUNT;
				if (schedule_task(&info->tqueue) == 0)
					MOD_DEC_USE_COUNT;
#else
# if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
		    	queue_task(&info->tqueue, &tq_scheduler);
# else
		    	queue_task_irq_off(&info->tqueue, &tq_scheduler);
# endif
#endif
        	}
	    } else {
			if ( !(status & UART_MSR_CTS) ){
		    	info->tty->hw_stopped = 1;
		    	if ((info->type != PORT_16550A) && (!info->IsMoxaMustChipFlag)) {
					info->IER &= ~UART_IER_THRI;
					outb(info->IER, info->base + UART_IER);
		    	}
			}
	    }
	}
}

static int mxser_block_til_ready(struct tty_struct *tty, struct file * filp,
				 struct mxser_struct *info)
{
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,4,0))
	DECLARE_WAITQUEUE(wait, current);
#else
	struct wait_queue	wait = { current, NULL };
#endif
	unsigned long 		flags;
	int			retval;
	int			do_clocal = 0;

	/*
	 * If the device is in the middle of being closed, then block
	 * until it's done, and then try again.
	 */
	if ( tty_hung_up_p(filp) || (info->flags & ASYNC_CLOSING) ) {
	    if ( info->flags & ASYNC_CLOSING )
		interruptible_sleep_on(&info->close_wait);
#ifdef SERIAL_DO_RESTART
	    if ( info->flags & ASYNC_HUP_NOTIFY )
		return(-EAGAIN);
	    else
		return(-ERESTARTSYS);
#else
	    return(-EAGAIN);
#endif
	}

	/*
	 * If this is a callout device, then just make sure the normal
	 * device isn't being used.
	 */
	if ( tty->driver.subtype == SERIAL_TYPE_CALLOUT ) {
	    if ( info->flags & ASYNC_NORMAL_ACTIVE )
		return(-EBUSY);
	    if ( (info->flags & ASYNC_CALLOUT_ACTIVE) &&
		 (info->flags & ASYNC_SESSION_LOCKOUT) &&
		 (info->session != current->session) )
		return(-EBUSY);
	    if ( (info->flags & ASYNC_CALLOUT_ACTIVE) &&
		 (info->flags & ASYNC_PGRP_LOCKOUT) &&
		 (info->pgrp != current->pgrp) )
		return(-EBUSY);
	    info->flags |= ASYNC_CALLOUT_ACTIVE;
	    return(0);
	}

	/*
	 * If non-blocking mode is set, or the port is not enabled,
	 * then make the check up front and then exit.
	 */
	if ( (filp->f_flags & O_NONBLOCK) ||
	     (tty->flags & (1 << TTY_IO_ERROR)) ) {
	    if ( info->flags & ASYNC_CALLOUT_ACTIVE )
		return(-EBUSY);
	    info->flags |= ASYNC_NORMAL_ACTIVE;
	    return(0);
	}

	if ( info->flags & ASYNC_CALLOUT_ACTIVE ) {
	    if ( info->normal_termios.c_cflag & CLOCAL )
		do_clocal = 1;
	} else {
	    if ( tty->termios->c_cflag & CLOCAL )
		do_clocal = 1;
	}

	/*
	 * Block waiting for the carrier detect and the line to become
	 * free (i.e., not in use by the callout).  While we are in
	 * this loop, info->count is dropped by one, so that
	 * mxser_close() knows when to free things.  We restore it upon
	 * exit, either normal or abnormal.
	 */
	retval = 0;
	add_wait_queue(&info->open_wait, &wait);
	save_flags(flags);
	cli();
	if ( !tty_hung_up_p(filp) )
	    info->count--;
	restore_flags(flags);
	info->blocked_open++;
	while ( 1 ) {
	    save_flags(flags);
	    cli();
	    if ( !(info->flags & ASYNC_CALLOUT_ACTIVE) )
		outb(inb(info->base + UART_MCR) | UART_MCR_DTR | UART_MCR_RTS,
		     info->base + UART_MCR);
	    restore_flags(flags);
	    current->state = TASK_INTERRUPTIBLE;
	    if ( tty_hung_up_p(filp) || !(info->flags & ASYNC_INITIALIZED) ) {
#ifdef SERIAL_DO_RESTART
		if ( info->flags & ASYNC_HUP_NOTIFY )
		    retval = -EAGAIN;
		else
		    retval = -ERESTARTSYS;
#else
		retval = -EAGAIN;
#endif
		break;
	    }
	    if ( !(info->flags & ASYNC_CALLOUT_ACTIVE) &&
		 !(info->flags & ASYNC_CLOSING) &&
		 (do_clocal || (inb(info->base + UART_MSR) & UART_MSR_DCD)) )
		break;
	    if ( signal_pending(current) ) {
		retval = -ERESTARTSYS;
		break;
	    }
	    schedule();
	}
	current->state = TASK_RUNNING;
	remove_wait_queue(&info->open_wait, &wait);
	if ( !tty_hung_up_p(filp) )
	    info->count++;
	info->blocked_open--;
	if ( retval )
	    return(retval);
	info->flags |= ASYNC_NORMAL_ACTIVE;
	return(0);
}

static int mxser_startup(struct mxser_struct * info)
{
	unsigned long	flags;
	unsigned long	page;

	page = get_free_page(GFP_KERNEL);
	if ( !page )
	    return(-ENOMEM);

	save_flags(flags);
	cli();

	if ( info->flags & ASYNC_INITIALIZED ) {
	    free_page(page);
	    restore_flags(flags);
	    return(0);
	}

	if ( !info->base || !info->type ) {
	    if ( info->tty )
		set_bit(TTY_IO_ERROR, &info->tty->flags);
	    free_page(page);
	    restore_flags(flags);
	    return(0);
	}
	if ( info->xmit_buf )
	    free_page(page);
	else
	    info->xmit_buf = (unsigned char *)page;

	/*
	 * Clear the FIFO buffers and disable them
	 * (they will be reenabled in mxser_change_speed())
	 */
	if ( info->xmit_fifo_size == 16 )
	    outb((UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT),
		 info->base + UART_FCR);
	// following add by Victor Yu. 08-30-2002
	else if ( info->xmit_fifo_size == 64 || info->xmit_fifo_size == ID1_XMIT_SIZE )
	    outb((UART_FCR_CLEAR_RCVR|UART_FCR_CLEAR_XMIT|MOXA_MUST_FCR_GDA_MODE_ENABLE), info->base+UART_FCR);
	// above add by Victor Yu. 08-30-2002

	/*
	 * At this point there's no way the LSR could still be 0xFF;
	 * if it is, then bail out, because there's likely no UART
	 * here.
	 */
	if ( inb(info->base + UART_LSR) == 0xff ) {
	    restore_flags(flags);
#if (LINUX_VERSION_CODE < VERSION_CODE(2,1,0))
	    if ( suser() ) {
#else
            if (capable(CAP_SYS_ADMIN)) {
#endif
		if ( info->tty )
		    set_bit(TTY_IO_ERROR, &info->tty->flags);
		return(0);
	    } else
		return(-ENODEV);
	}

	/*
	 * Clear the interrupt registers.
	 */
	(void)inb(info->base + UART_LSR);
	(void)inb(info->base + UART_RX);
	(void)inb(info->base + UART_IIR);
	(void)inb(info->base + UART_MSR);

	/*
	 * Now, initialize the UART
	 */
	outb(UART_LCR_WLEN8, info->base + UART_LCR);	/* reset DLAB */
	info->MCR = UART_MCR_DTR | UART_MCR_RTS;
	outb(info->MCR, info->base + UART_MCR);

	/*
	 * Finally, enable interrupts
	 */
//	info->IER = UART_IER_MSI | UART_IER_RLSI | UART_IER_RDI;
	info->IER = UART_IER_RLSI | UART_IER_RDI;

	// following add by Victor Yu. 08-30-2002
	if ( info->IsMoxaMustChipFlag )
		info->IER |= MOXA_MUST_IER_EGDAI;
	// above add by Victor Yu. 08-30-2002
	outb(info->IER, info->base + UART_IER); /* enable interrupts */

	/*
	 * And clear the interrupt registers again for luck.
	 */
	(void)inb(info->base + UART_LSR);
	(void)inb(info->base + UART_RX);
	(void)inb(info->base + UART_IIR);
	(void)inb(info->base + UART_MSR);

#if (LINUX_VERSION_CODE <  VERSION_CODE(2,1,0))
	if ( info->tty )
	    clear_bit(TTY_IO_ERROR, &info->tty->flags);
#else
	if ( info->tty )
	    test_and_clear_bit(TTY_IO_ERROR, &info->tty->flags);
#endif
	info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;

	/*
	 * and set the speed of the serial port
	 */
	mxser_change_speed(info, 0);

	info->flags |= ASYNC_INITIALIZED;
	restore_flags(flags);
	return(0);
}

/*
 * This routine will shutdown a serial port; interrupts maybe disabled, and
 * DTR is dropped if the hangup on close termio flag is on.
 */
static void mxser_shutdown(struct mxser_struct * info)
{
	unsigned long	flags;

	if ( !(info->flags & ASYNC_INITIALIZED) )
	    return;

	save_flags(flags);
	cli();			/* Disable interrupts */

	/*
	 * clear delta_msr_wait queue to avoid mem leaks: we may free the irq
	 * here so the queue might never be waken up
	 */
	wake_up_interruptible(&info->delta_msr_wait);

	/*
	 * Free the IRQ, if necessary
	 */
	if ( info->xmit_buf ) {
	    free_page((unsigned long)info->xmit_buf);
	    info->xmit_buf = 0;
	}

	info->IER = 0;
	outb(0x00, info->base + UART_IER);

	if ( !info->tty || (info->tty->termios->c_cflag & HUPCL) )
	    info->MCR &= ~(UART_MCR_DTR | UART_MCR_RTS);
	outb(info->MCR, info->base + UART_MCR);

	/* clear Rx/Tx FIFO's */
	// following add by Victor Yu. 08-30-2002
	if ( info->IsMoxaMustChipFlag )
		outb((UART_FCR_CLEAR_RCVR|UART_FCR_CLEAR_XMIT|MOXA_MUST_FCR_GDA_MODE_ENABLE), info->base + UART_FCR);
	else
	// above add by Victor Yu. 08-30-2002
		outb((UART_FCR_CLEAR_RCVR|UART_FCR_CLEAR_XMIT), info->base + UART_FCR);

	/* read data port to reset things */
	(void)inb(info->base + UART_RX);

	if ( info->tty )
	    set_bit(TTY_IO_ERROR, &info->tty->flags);

	info->flags &= ~ASYNC_INITIALIZED;

	// following add by Victor Yu. 09-23-2002
	if ( info->IsMoxaMustChipFlag ) {
		SET_MOXA_MUST_NO_SOFTWARE_FLOW_CONTROL(info->base);
	}
	// above add by Victor Yu. 09-23-2002

	restore_flags(flags);
}

/*
 * This routine is called to set the UART divisor registers to match
 * the specified baud rate for a serial port.
 */
static int mxser_change_speed(struct mxser_struct *info,
                              struct termios *old_termios)
{
	unsigned	cflag, cval, fcr;
        int             ret = 0;
	unsigned long	flags;
	unsigned char status;
	long baud;


	if ( !info->tty || !info->tty->termios )
	    return ret;
	cflag = info->tty->termios->c_cflag;
	if ( !(info->base) )
	    return ret;


#ifndef B921600
#define B921600 (B460800 +1)
#endif
	if( mxser_set_baud_method[info->port] == 0)
	{
		switch( cflag & (CBAUD | CBAUDEX) ){
        	case B921600 : baud = 921600;	break;
        	case B460800 : baud = 460800;	break;
        	case B230400 : baud = 230400;	break;
        	case B115200 : baud = 115200;	break;
        	case B57600 : baud = 57600;	break;
        	case B38400 : baud = 38400;	break;
        	case B19200 : baud = 19200;	break;
        	case B9600 : baud = 9600;	break;
        	case B4800 : baud = 4800;	break;
        	case B2400 : baud = 2400;	break;
        	case B1800 : baud = 1800;	break;
        	case B1200 : baud = 1200;	break;
        	case B600 : baud = 600;	break;
        	case B300 : baud = 300;	break;
        	case B200 : baud = 200;	break;
        	case B150 : baud = 150;	break;
        	case B134 : baud = 134;	break;
        	case B110 : baud = 110;	break;
        	case B75 : baud = 75;	break;
        	case B50 : baud = 50;	break;
        	default: baud = 0;	break;
		}
       	mxser_set_baud(info, baud);
	}

	/* byte size and parity */
	switch ( cflag & CSIZE ) {
	case CS5: cval = 0x00; break;
	case CS6: cval = 0x01; break;
	case CS7: cval = 0x02; break;
	case CS8: cval = 0x03; break;
	default:  cval = 0x00; break;	/* too keep GCC shut... */
	}
	if ( cflag & CSTOPB )
	    cval |= 0x04;
	if ( cflag & PARENB )
	    cval |= UART_LCR_PARITY;
#ifndef CMSPAR
#define	CMSPAR 010000000000
#endif
	if ( !(cflag & PARODD) ){
	    cval |= UART_LCR_EPAR;
	}
	if ( cflag & CMSPAR )
	   cval |= UART_LCR_SPAR;

	if ( (info->type == PORT_8250) || (info->type == PORT_16450) ) {
	    if ( info->IsMoxaMustChipFlag ) {
			fcr = UART_FCR_ENABLE_FIFO;
			fcr |= MOXA_MUST_FCR_GDA_MODE_ENABLE;
			SET_MOXA_MUST_FIFO_VALUE(info);
	    }else
	    	fcr = 0;
	} else {
	    fcr = UART_FCR_ENABLE_FIFO;
	    // following add by Victor Yu. 08-30-2002
	    if ( info->IsMoxaMustChipFlag ) {
			fcr |= MOXA_MUST_FCR_GDA_MODE_ENABLE;
			SET_MOXA_MUST_FIFO_VALUE(info);
	    } else {
	    // above add by Victor Yu. 08-30-2002

	    	switch ( info->rx_trigger ) {
	    		case 1:  fcr |= UART_FCR_TRIGGER_1; break;
	    		case 4:  fcr |= UART_FCR_TRIGGER_4; break;
	    		case 8:  fcr |= UART_FCR_TRIGGER_8; break;
	    		default: fcr |= UART_FCR_TRIGGER_14;
	    	}
	    }
	}

	/* CTS flow control flag and modem status interrupts */
	info->IER &= ~UART_IER_MSI;
	info->MCR &= ~UART_MCR_AFE;
	if ( cflag & CRTSCTS ) {
	    info->flags |= ASYNC_CTS_FLOW;
//	    info->IER |= UART_IER_MSI;
	    if (( info->type == PORT_16550A ) || (info->IsMoxaMustChipFlag)){
			info->MCR |= UART_MCR_AFE;
			status = mxser_get_msr(info->base, 0, info->port);
			mxser_check_modem_status(info, status);
		}else {
			status = mxser_get_msr(info->base, 0, info->port);
			if (info->tty->hw_stopped) {
				if (status & UART_MSR_CTS) {
					info->tty->hw_stopped = 0;
					if ((info->type != PORT_16550A) && (!info->IsMoxaMustChipFlag)){
						info->IER |= UART_IER_THRI;
						outb(info->IER, info->base + UART_IER);
					}
					set_bit(MXSER_EVENT_TXLOW, &info->event);
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,4,0))
					MOD_INC_USE_COUNT;
					if (schedule_task(&info->tqueue) == 0)
						MOD_DEC_USE_COUNT;
#else
					queue_task(&info->tqueue, &tq_scheduler);
#endif
				}
			} else {
				if (!(status & UART_MSR_CTS)) {
					info->tty->hw_stopped = 1;
					if ((info->type != PORT_16550A) && (!info->IsMoxaMustChipFlag)){
						info->IER &= ~UART_IER_THRI;
						outb(info->IER, info->base + UART_IER);
					}
				}
			}
		}
	} else {
	    info->flags &= ~ASYNC_CTS_FLOW;
	}
	outb(info->MCR, info->base + UART_MCR);
	if ( cflag & CLOCAL ){
	    info->flags &= ~ASYNC_CHECK_CD;
	}else {
	    info->flags |= ASYNC_CHECK_CD;
//	    info->IER |= UART_IER_MSI;
	}
	outb(info->IER, info->base + UART_IER);

	/*
	 * Set up parity check flag
	 */
	info->read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
	if ( I_INPCK(info->tty) )
	    info->read_status_mask |= UART_LSR_FE | UART_LSR_PE;
	if ( I_BRKINT(info->tty) || I_PARMRK(info->tty) )
	    info->read_status_mask |= UART_LSR_BI;

	info->ignore_status_mask = 0;
#if 0
	/* This should be safe, but for some broken bits of hardware... */
	if ( I_IGNPAR(info->tty) ) {
	    info->ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
	    info->read_status_mask |= UART_LSR_PE | UART_LSR_FE;
	}
#endif
	if ( I_IGNBRK(info->tty) ) {
	    info->ignore_status_mask |= UART_LSR_BI;
	    info->read_status_mask |= UART_LSR_BI;
	    /*
	     * If we're ignore parity and break indicators, ignore
	     * overruns too.  (For real raw support).
	     */
	    if ( I_IGNPAR(info->tty) ) {
		info->ignore_status_mask |= UART_LSR_OE|UART_LSR_PE|UART_LSR_FE;
		info->read_status_mask |= UART_LSR_OE|UART_LSR_PE|UART_LSR_FE;
	    }
	}

	// following add by Victor Yu. 09-02-2002
	if ( info->IsMoxaMustChipFlag ) {
		save_flags(flags);
		cli();
		SET_MOXA_MUST_XON1_VALUE(info->base, START_CHAR(info->tty));
		SET_MOXA_MUST_XOFF1_VALUE(info->base, STOP_CHAR(info->tty));
		if ( I_IXON(info->tty) ) {
			ENABLE_MOXA_MUST_RX_SOFTWARE_FLOW_CONTROL(info->base);
		} else {
			DISABLE_MOXA_MUST_RX_SOFTWARE_FLOW_CONTROL(info->base);
		}
		if ( I_IXOFF(info->tty) ) {
			ENABLE_MOXA_MUST_TX_SOFTWARE_FLOW_CONTROL(info->base);
		} else {
			DISABLE_MOXA_MUST_TX_SOFTWARE_FLOW_CONTROL(info->base);
		}
		/*
		if ( I_IXANY(info->tty) ) {
			info->MCR |= MOXA_MUST_MCR_XON_ANY;
			ENABLE_MOXA_MUST_XON_ANY_FLOW_CONTROL(info->base);
		} else {
			info->MCR &= ~MOXA_MUST_MCR_XON_ANY;
			DISABLE_MOXA_MUST_XON_ANY_FLOW_CONTROL(info->base);
		}
		*/
		restore_flags(flags);
	}
	// above add by Victor Yu. 09-02-2002


	save_flags(flags);
	cli();
	outb(fcr, info->base + UART_FCR);		    /* set fcr */
	outb(cval, info->base + UART_LCR);  

	restore_flags(flags);

        return ret;
}


static int mxser_set_baud(struct mxser_struct *info, long newspd)
{
	int		quot = 0;
	unsigned char	cval;
        int             ret = 0;
	unsigned long	flags;

	if ( !info->tty || !info->tty->termios )
	    return ret;

	if ( !(info->base) )
	    return ret;

    if ( newspd > info->MaxCanSetBaudRate )
	    return 0;

    info->realbaud = newspd;
	if ( newspd == 134 ) {
	    quot = (2 * info->baud_base / 269);
	} else if ( newspd ) {
	    quot = info->baud_base / newspd;

	    if(quot==0)
	        quot = 1;

	} else {
	    quot = 0;
	}

	info->timeout = ((info->xmit_fifo_size*HZ*10*quot) / info->baud_base);
	info->timeout += HZ/50;		/* Add .02 seconds of slop */

	if ( quot ) {
	    info->MCR |= UART_MCR_DTR;
	    save_flags(flags);
	    cli();
	    outb(info->MCR, info->base + UART_MCR);
	    restore_flags(flags);
	} else {
	    info->MCR &= ~UART_MCR_DTR;
	    save_flags(flags);
	    cli();
	    outb(info->MCR, info->base + UART_MCR);
	    restore_flags(flags);
	    return ret;
	}

    cval = inb(info->base + UART_LCR);

	save_flags(flags);
	cli();
	outb(cval | UART_LCR_DLAB, info->base + UART_LCR);  /* set DLAB */

	outb(quot & 0xff, info->base + UART_DLL);	    /* LS of divisor */
	outb(quot >> 8, info->base + UART_DLM); 	    /* MS of divisor */
	outb(cval, info->base + UART_LCR);		    /* reset DLAB */

	restore_flags(flags);

    return ret;
}



/*
 * ------------------------------------------------------------
 * friends of mxser_ioctl()
 * ------------------------------------------------------------
 */
static int mxser_get_serial_info(struct mxser_struct * info,
				 struct serial_struct * retinfo)
{
	struct serial_struct	tmp;

	if ( !retinfo )
	    return(-EFAULT);
	memset(&tmp, 0, sizeof(tmp));
	tmp.type = info->type;
	tmp.line = info->port;
	tmp.port = info->base;
	tmp.irq = info->irq;
	tmp.flags = info->flags;
	tmp.baud_base = info->baud_base;
	tmp.close_delay = info->close_delay;
	tmp.closing_wait = info->closing_wait;
	tmp.custom_divisor = info->custom_divisor;
	tmp.hub6 = 0;
	copy_to_user(retinfo, &tmp, sizeof(*retinfo));
	return(0);
}

static int mxser_set_serial_info(struct mxser_struct * info,
				 struct serial_struct * new_info)
{
	struct serial_struct	new_serial;
	unsigned int		flags;
	int			retval = 0;

	if ( !new_info || !info->base )
	    return(-EFAULT);
	copy_from_user(&new_serial, new_info, sizeof(new_serial));

	if ( (new_serial.irq != info->irq) ||
	     (new_serial.port != info->base) ||
	     (new_serial.custom_divisor != info->custom_divisor) ||
	     (new_serial.baud_base != info->baud_base) )
	    return(-EPERM);

	flags = info->flags & ASYNC_SPD_MASK;

#if (LINUX_VERSION_CODE < VERSION_CODE(2,1,0))
       if ( !suser() ) {
#else
       if ( !capable(CAP_SYS_ADMIN)) {
#endif
	    if ( (new_serial.baud_base != info->baud_base) ||
		 (new_serial.close_delay != info->close_delay) ||
		 ((new_serial.flags & ~ASYNC_USR_MASK) !=
		 (info->flags & ~ASYNC_USR_MASK)) )
		return(-EPERM);
	    info->flags = ((info->flags & ~ASYNC_USR_MASK) |
			  (new_serial.flags & ASYNC_USR_MASK));
	} else {
	    /*
	     * OK, past this point, all the error checking has been done.
	     * At this point, we start making changes.....
	     */
	    info->flags = ((info->flags & ~ASYNC_FLAGS) |
			  (new_serial.flags & ASYNC_FLAGS));
	    info->close_delay = new_serial.close_delay * HZ/100;
	    info->closing_wait = new_serial.closing_wait * HZ/100;
#if (LINUX_VERSION_CODE >= VERSION_CODE(2,1,0))
	    info->tty->low_latency = (info->flags & ASYNC_LOW_LATENCY) ? 1 : 0;
	    info->tty->low_latency = 0;//(info->flags & ASYNC_LOW_LATENCY) ? 1 : 0;
#endif
	}

/* added by casper, 3/17/2000, for mouse */
	info->type = new_serial.type;

	if(info->type == PORT_16550A){
		// following add by Victor Yu. 09-02-2002
	// if ( info->IsMoxaMustChipFlag )	// mask by Victor Yu. 02-10-2004
		if ( info->IsMoxaMustChipFlag == MOXA_MUST_HARDWARE_ID ) {
			info->xmit_fifo_size = 64;
			info->rx_trigger = 56;
		} else if ( info->IsMoxaMustChipFlag == MOXA_MUST_HARDWARE_ID1 ) {
			info->xmit_fifo_size = ID1_XMIT_SIZE;
			info->rx_trigger = ID1_RX_TRIG;
		}else{
		// above add by Victor Yu. 09-02-2002
			info->xmit_fifo_size = 16;
			info->rx_trigger = 14;
		}
	}else
		info->xmit_fifo_size = 1;

/* */
	if ( info->flags & ASYNC_INITIALIZED ) {
	    if ( flags != (info->flags & ASYNC_SPD_MASK) ){
		mxser_change_speed(info,0);
	    }
	} else{
	    retval = mxser_startup(info);
	}
	return(retval);
}

/*
 * mxser_get_lsr_info - get line status register info
 *
 * Purpose: Let user call ioctl() to get info when the UART physically
 *	    is emptied.  On bus types like RS485, the transmitter must
 *	    release the bus after transmitting. This must be done when
 *	    the transmit shift register is empty, not be done when the
 *	    transmit holding register is empty.  This functionality
 *	    allows an RS485 driver to be written in user space.
 */
static int mxser_get_lsr_info(struct mxser_struct * info, unsigned int *value)
{
	unsigned char	status;
	unsigned int	result;
	unsigned long	flags;

	save_flags(flags);
	cli();
	status = inb(info->base + UART_LSR);
	restore_flags(flags);
	result = ((status & UART_LSR_TEMT) ? TIOCSER_TEMT : 0);
	put_to_user(result, value);
	return(0);
}

/*
 * This routine sends a break character out the serial port.
 */
static void mxser_send_break(struct mxser_struct * info, int duration)
{
	unsigned long	flags;

	if ( !info->base )
	    return;
	current->state = TASK_INTERRUPTIBLE;
	save_flags(flags);
	cli();
	outb(inb(info->base + UART_LCR) | UART_LCR_SBC, info->base + UART_LCR);
	schedule_timeout(duration);
	outb(inb(info->base + UART_LCR) & ~UART_LCR_SBC, info->base + UART_LCR);
	restore_flags(flags);
}

static int mxser_get_modem_info(struct mxser_struct * info,
				unsigned int *value)
{
	unsigned char	control, status;
	unsigned int	result;

	control = info->MCR;
	status = mxser_get_msr(info->base, 0, info->port);

	if ( status & UART_MSR_ANY_DELTA )
	    mxser_check_modem_status(info, status);
	result = ((control & UART_MCR_RTS) ? TIOCM_RTS : 0) |
		 ((control & UART_MCR_DTR) ? TIOCM_DTR : 0) |
		 ((status  & UART_MSR_DCD) ? TIOCM_CAR : 0) |
		 ((status  & UART_MSR_RI)  ? TIOCM_RNG : 0) |
		 ((status  & UART_MSR_DSR) ? TIOCM_DSR : 0) |
		 ((status  & UART_MSR_CTS) ? TIOCM_CTS : 0);
	put_to_user(result, value);
	return(0);
}

static int mxser_set_modem_info(struct mxser_struct * info, unsigned int cmd,
				unsigned int *value)
{
	int		error;
	unsigned int	arg;
	unsigned long	flags;

	error = verify_area(VERIFY_READ, value, sizeof(int));
	if ( error )
	    return(error);
	get_from_user(arg,value);
	switch ( cmd ) {
	case TIOCMBIS:
	    if ( arg & TIOCM_RTS )
		info->MCR |= UART_MCR_RTS;
	    if ( arg & TIOCM_DTR )
		info->MCR |= UART_MCR_DTR;
	    break;
	case TIOCMBIC:
	    if ( arg & TIOCM_RTS )
		info->MCR &= ~UART_MCR_RTS;
	    if ( arg & TIOCM_DTR )
		info->MCR &= ~UART_MCR_DTR;
	    break;
	case TIOCMSET:
	    info->MCR = ((info->MCR & ~(UART_MCR_RTS | UART_MCR_DTR)) |
			((arg & TIOCM_RTS) ? UART_MCR_RTS : 0) |
			((arg & TIOCM_DTR) ? UART_MCR_DTR : 0));
	    break;
	default:
	    return(-EINVAL);
	}
	save_flags(flags);
	cli();
	outb(info->MCR, info->base + UART_MCR);
	restore_flags(flags);
	return(0);
}

static int	mxser_read_register(int, unsigned short *);
static int	mxser_program_mode(int);
static void	mxser_normal_mode(int);

static int mxser_get_ISA_conf(int cap,struct mxser_hwconf *hwconf)
{
	int		id, i, bits;
	unsigned short	regs[16], irq;
	unsigned char	scratch, scratch2;

	id = mxser_read_register(cap, regs);
	if (id == C168_ASIC_ID){
	    hwconf->board_type = MXSER_BOARD_C168_ISA;
	    hwconf->ports = 8;
	}else if (id == C104_ASIC_ID){
	    hwconf->board_type = MXSER_BOARD_C104_ISA;
	    hwconf->ports = 4;
	}else if (id == C102_ASIC_ID){
	    hwconf->board_type = MXSER_BOARD_C102_ISA;
	    hwconf->ports = 2;
	}else if (id == CI132_ASIC_ID){
	    hwconf->board_type = MXSER_BOARD_CI132;
	    hwconf->ports = 2;
	}else if (id == CI134_ASIC_ID){
	    hwconf->board_type = MXSER_BOARD_CI134;
	    hwconf->ports = 4;
	}else if (id == CI104J_ASIC_ID){
	    hwconf->board_type = MXSER_BOARD_CI104J;
	    hwconf->ports = 4;
        }else
	    return(0);

	irq = 0;
	if(hwconf->ports==2){
	    irq = regs[9] & 0xF000;
	    irq = irq | (irq>>4);
	    if (irq != (regs[9] & 0xFF00))
	        return(MXSER_ERR_IRQ_CONFLIT);
	}else if (hwconf->ports==4){
	    irq = regs[9] & 0xF000;
	    irq = irq | (irq>>4);
	    irq = irq | (irq>>8);
	    if (irq != regs[9])
	        return(MXSER_ERR_IRQ_CONFLIT);
	}else if (hwconf->ports==8){
	    irq = regs[9] & 0xF000;
	    irq = irq | (irq>>4);
	    irq = irq | (irq>>8);
	    if ((irq != regs[9]) || (irq != regs[10]))
	        return(MXSER_ERR_IRQ_CONFLIT);
        }

	if ( !irq ) {
	    return(MXSER_ERR_IRQ);
	}
	hwconf->irq = ((int)(irq & 0xF000) >>12);
	for ( i=0; i<8; i++ )
	    hwconf->ioaddr[i] = (int)regs[i + 1] & 0xFFF8;
	if ( (regs[12] & 0x80) == 0 ) {
	    return(MXSER_ERR_VECTOR);
	}
	hwconf->vector = (int)regs[11];		/* interrupt vector */
	if ( id == 1 )
	    hwconf->vector_mask = 0x00FF;
	else
	    hwconf->vector_mask = 0x000F;
	for ( i=7, bits=0x0100; i>=0; i--, bits <<= 1 ) {
	    if ( regs[12] & bits ) {
		hwconf->baud_base[i] = 921600;
		hwconf->MaxCanSetBaudRate[i] = 921600;	// add by Victor Yu. 09-04-2002
	    } else {
		hwconf->baud_base[i] = 115200;
		hwconf->MaxCanSetBaudRate[i] = 115200;	// add by Victor Yu. 09-04-2002
	    }
	}
	scratch2 = inb(cap + UART_LCR) & (~UART_LCR_DLAB);
	outb(scratch2 | UART_LCR_DLAB, cap + UART_LCR);
	outb(0, cap + UART_EFR);	/* EFR is the same as FCR */
	outb(scratch2, cap + UART_LCR);
	outb(UART_FCR_ENABLE_FIFO, cap + UART_FCR);
	scratch = inb(cap + UART_IIR);

	if ( scratch & 0xC0 )
	    hwconf->uart_type = PORT_16550A;
	else
	    hwconf->uart_type = PORT_16450;
	if ( id == 1 )
	    hwconf->ports = 8;
	else
	    hwconf->ports = 4;
	return(hwconf->ports);
}

#define CHIP_SK 	0x01		/* Serial Data Clock  in Eprom */
#define CHIP_DO 	0x02		/* Serial Data Output in Eprom */
#define CHIP_CS 	0x04		/* Serial Chip Select in Eprom */
#define CHIP_DI 	0x08		/* Serial Data Input  in Eprom */
#define EN_CCMD 	0x000		/* Chip's command register     */
#define EN0_RSARLO	0x008		/* Remote start address reg 0  */
#define EN0_RSARHI	0x009		/* Remote start address reg 1  */
#define EN0_RCNTLO	0x00A		/* Remote byte count reg WR    */
#define EN0_RCNTHI	0x00B		/* Remote byte count reg WR    */
#define EN0_DCFG	0x00E		/* Data configuration reg WR   */
#define EN0_PORT	0x010		/* Rcv missed frame error counter RD */
#define ENC_PAGE0	0x000		/* Select page 0 of chip registers   */
#define ENC_PAGE3	0x0C0		/* Select page 3 of chip registers   */
static int mxser_read_register(int port, unsigned short *regs)
{
	int		i, k, value, id;
	unsigned int	j;

	id = mxser_program_mode(port);
	if ( id < 0 )
	    return(id);
	for ( i=0; i<14; i++ ) {
	    k = (i & 0x3F) | 0x180;
	    for ( j=0x100; j>0; j>>=1 ) {
		outb(CHIP_CS, port);
		if ( k & j ) {
		    outb(CHIP_CS | CHIP_DO, port);
		    outb(CHIP_CS | CHIP_DO | CHIP_SK, port);	/* A? bit of read */
		} else {
		    outb(CHIP_CS, port);
		    outb(CHIP_CS | CHIP_SK, port);	/* A? bit of read */
		}
	    }
	    (void)inb(port);
	    value = 0;
	    for ( k=0, j=0x8000; k<16; k++, j>>=1 ) {
		outb(CHIP_CS, port);
		outb(CHIP_CS | CHIP_SK, port);
		if ( inb(port) & CHIP_DI )
		    value |= j;
	    }
	    regs[i] = value;
	    outb(0, port);
	}
	mxser_normal_mode(port);
	return(id);
}

static int mxser_program_mode(int port)
{
	int	id, i, j, n;
	unsigned long	flags;

	save_flags(flags);
	cli();
	outb(0, port);
	outb(0, port);
	outb(0, port);
	(void)inb(port);
	(void)inb(port);
	outb(0, port);
	(void)inb(port);
	restore_flags(flags);
	id = inb(port + 1) & 0x1F;
	if ( (id != C168_ASIC_ID) &&
		(id != C104_ASIC_ID) &&
		(id != C102_ASIC_ID) &&
		(id != CI132_ASIC_ID) &&
		(id != CI134_ASIC_ID) &&
		(id != CI104J_ASIC_ID) )
	    return(-1);
	for ( i=0, j=0; i<4; i++ ) {
	    n = inb(port + 2);
	    if ( n == 'M' ) {
		j = 1;
	    } else if ( (j == 1) && (n == 1) ) {
		j = 2;
		break;
	    } else
		j = 0;
	}
	if ( j != 2 )
	    id = -2;
	return(id);
}

static void mxser_normal_mode(int port)
{
	int	i, n;

	outb(0xA5, port + 1);
	outb(0x80, port + 3);
	outb(12, port + 0);		    /* 9600 bps */
	outb(0, port + 1);
	outb(0x03, port + 3);		    /* 8 data bits */
	outb(0x13, port + 4);		    /* loop back mode */
	for ( i=0; i<16; i++ ) {
	    n = inb(port + 5);
	    if ( (n & 0x61) == 0x60 )
		break;
	    if ( (n & 1) == 1 )
		(void)inb(port);
	}
	outb(0x00, port + 4);
}

// added by James 03-05-2004.
// for secure device server:
// stat = 1, the port8 DTR is set to ON.
// stat = 0, the port8 DTR is set to OFF.
void SDS_PORT8_DTR(int stat)
{
        int _sds_oldmcr;
	    _sds_oldmcr = inb(mxvar_table[7].base + UART_MCR);    // get old MCR
        if (stat == 1) {
    	    outb(_sds_oldmcr | 0x01, mxvar_table[7].base + UART_MCR);    // set DTR ON
        }
        if (stat == 0) {
    	    outb(_sds_oldmcr & 0xfe, mxvar_table[7].base + UART_MCR);    // set DTR OFF
        }
        return;
}
#ifndef MODULE
EXPORT_SYMBOL(SDS_PORT8_DTR);
#endif
// (above) added by James.
