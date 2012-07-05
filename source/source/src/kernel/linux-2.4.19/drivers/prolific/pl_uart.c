/*********************************************************************
 * Filename:      pl_uart.c
 * Version:       0.1
 * Description:   PL-1061B UART driver
 * Created at:    Sep 18 2003
 *
 * #001 2006/05/17  jedy    1.0.1 add 120MHz dclk supporting
 ********************************************************************/
/*
static char                       *serial_version = "1.0.1";
static char                       *serial_revdate = "2006-04-21";
*/
static char                       *serial_version = "1.1.0";
static char                       *serial_revdate = "2007-02-15";

#include <linux/config.h>
#include <linux/version.h>
#include <linux/kernel_stat.h>

#undef SERIAL_PARANOIA_CHECK
#define CONFIG_SERIAL_NOPAUSE_IO
#define SERIAL_DO_RESTART

#define PL_UART_DEBUG
#undef PL_UART_DEBUG
#define DMA_IN_OPEN 0
#define DMA_OUT_OPEN 0

#ifndef PL_UART_DEBUG
  #undef SERIAL_DEBUG_INTR
  #undef SERIAL_DEBUG_OPEN
  #undef SERIAL_DEBUG_IOCTL
  #undef SERIAL_DEBUG_WRITE
  #undef SERIAL_DEBUG_READ
  #undef SERIAL_DEBUG_FLOW
  #undef SERIAL_DEBUG_THROTTLE
  #undef SERIAL_DEBUG_RS_WAIT_UNTIL_SENT
  #undef SERIAL_DEBUG_PCI
#else
  #define SERIAL_DEBUG_INTR
  #define SERIAL_DEBUG_OPEN
  #define SERIAL_DEBUG_IOCTL
  #define SERIAL_DEBUG_WRITE
  #define SERIAL_DEBUG_READ
  #define SERIAL_DEBUG_FLOW
  #define SERIAL_DEBUG_THROTTLE
  #define SERIAL_DEBUG_RS_WAIT_UNTIL_SENT
  #define SERIAL_DEBUG_PCI
#endif

#define RS_STROBE_TIME		(10 * HZ)
#define RS_ISR_PASS_LIMIT 256

#include <linux/module.h>
#include <linux/types.h>
#include <linux/serial.h>
#include <linux/serialP.h>
#include <linux/serial_reg.h>

#define LOCAL_VERSTRING "-1"

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
#ifdef CONFIG_SERIAL_CONSOLE
  #include <linux/console.h>
#endif
#ifdef CONFIG_MAGIC_SYSRQ
  #include <linux/sysrq.h>
#endif

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/bitops.h>

#include <linux/pci.h>
#include <asm/ioctls.h>
//#include <asm/pl1061/dma.h>

#include "pl_uart.h"

#include "uart_regs.h"
#include "uart_utils.h"

/* PL1029 include file */
#include <asm/io.h>
#include <asm/atomic.h>
#include <asm/dma.h>
#include <asm/irq.h>
#include <asm/arch-pl1029/pl_symbol_alarm.h>
#include <linux/major.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/hardware.h>

/* PL1097 inclide file
#include <asm/arch-pl1091/pl_symbol_alarm.h>
#include <asm/arch/pl_dma.h>
#include <asm/arch/irqs.h>
#include <asm/pl_reg.h>
#include <asm/system.h>
#include <asm/pl_irq.h>
*/

#define ENABLE_UART_1 	0
#define RS485		1
#define WAKEUP_CHARS	256

//volatile UART_REGS                *UartBaseAddr = (UART_REGS *)PL_UART_BASE;

static char                       *serial_name = "PL UART driver";
static                            DECLARE_TASK_QUEUE(tq_serial);
static struct tty_driver          serial_driver, callout_driver;
static int                        serial_refcount;
static struct timer_list          serial_timer;
static struct async_struct        *IRQ_ports[NR_IRQS];
static int                        IRQ_timeout[NR_IRQS];

static struct serial_state        uart_table[2] = {
        {
            baud_base:          1,
            irq :               IRQ_PL_UART,
            flags :             PL_COM_FLAGS,
            type :              PORT_PL_UART,
            xmit_fifo_size :    2,
            iomem_base :        (u8 *)PL_UART_BASE,
            iomem_reg_shift :   0,
            io_type :           SERIAL_IO_MEM
        },

#if     ENABLE_UART_1
        {
            baud_base:          1,
            irq :               IRQ_PL_UART_1,
            flags :             PL_COM_FLAGS,
            type :              PORT_PL_UART_1,
            xmit_fifo_size :    2,
            iomem_base :        (u8 *)PL_UART_1_BASE,
            iomem_reg_shift :   0,
            io_type :           SERIAL_IO_MEM
        },
#endif
};

static struct serial_uart_config  uart_config[] =
{
  {"unknown", 1, 0},
  {"8250", 1, 0},
  {"16450", 1, 0},
  {"16550", 1, 0},
  {"16550A", 16, UART_CLEAR_FIFO | UART_USE_FIFO},
  {"cirrus", 1, 0},
  {"ST16650", 1, UART_CLEAR_FIFO | UART_STARTECH},
  {"ST16650V2", 32, UART_CLEAR_FIFO | UART_USE_FIFO | UART_STARTECH},
  {"TI16750", 64, UART_CLEAR_FIFO | UART_USE_FIFO},
  {"Startech", 1, 0},
  {"16C950/954", 128, UART_CLEAR_FIFO | UART_USE_FIFO},
  {"ST16654", 64, UART_CLEAR_FIFO | UART_USE_FIFO | UART_STARTECH},
  {"XR16850", 128, UART_CLEAR_FIFO | UART_USE_FIFO | UART_STARTECH},
  {"RSA", 2048, UART_CLEAR_FIFO | UART_USE_FIFO},
  {"PL CONSOLE", 4, UART_USE_FIFO},
  {"PL UART", 4, UART_USE_FIFO},
  {"PL UART1", 4, UART_USE_FIFO},
  {0, 0}
};

extern unsigned int               osci_freq;

#define NR_PORTS	(sizeof(uart_table) / sizeof(struct serial_state))
#ifndef PREPARE_FUNC
  #define PREPARE_FUNC(dev)			(dev->prepare)
  #define ACTIVATE_FUNC(dev)		(dev->activate)
  #define DEACTIVATE_FUNC(dev)	(dev->deactivate)
#endif

#define HIGH_BITS_OFFSET	((sizeof(long) - sizeof(int)) << 3)

static struct tty_struct          *serial_table[NR_PORTS];
static struct termios             *serial_termios[NR_PORTS];
static struct termios             *serial_termios_locked[NR_PORTS];

static void                       autoconfig(struct serial_state *state);
static void                       change_speed(struct async_struct *info, struct termios *old);
static void                       uart_wait_until_sent(struct tty_struct *tty, int timeout);
static unsigned                   detect_uart_irq(struct serial_state *state);

static unsigned char              *tmp_buf;

#ifdef DECLARE_MUTEX
static                            DECLARE_MUTEX(tmp_buf_sem);
#else
static struct semaphore           tmp_buf_sem = MUTEX;
#endif

#if DMA_OUT_OPEN
static unsigned char              *gDMAWriteBuffer; 
#endif
#if DMA_IN_OPEN
static unsigned char		  *gDMAReadBuffer;
#endif
static unsigned char  		  *gTmpBuffer;

static int                        HWFlowMode;

static inline int serial_paranoia_check(struct async_struct *info, kdev_t device, const char *routine)
{
#ifdef SERIAL_PARANOIA_CHECK
  static const char *badmagic = "Warning: bad magic number for serial struct (%s) in %s\n";
  static const char *badinfo = "Warning: null async_struct for (%s) in %s\n";

  if(!info) {
    printk(badinfo, kdevname(device), routine);
    return 1;
  }

  if(info->magic != SERIAL_MAGIC) {
    printk(badmagic, kdevname(device), routine);
    return 1;
  }
#endif
  return 0;
}

#if 0
static void GPIO_OutputEnable(unsigned int idx, int set)
{
  unsigned int regVal;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out("GPIO_OutputEnable - idx: %d, set: %d\n", idx, set);
#endif

  regVal = readl(GPIO_ENB);

  regVal |= idx;

  if(set)
    regVal &= ~(idx << 16);
  else
    regVal |= (idx << 16);

  writel(regVal, GPIO_ENB);
}
#endif

void ProcessDTR(struct async_struct *info)
{
#if 0
  if(info->MCR & UART_MCR_DTR)
    GPIO_OutputEnable(PIN_DTR, 1);
  else
    GPIO_OutputEnable(PIN_DTR, 0);
#endif
}

void ProcessRTS(struct async_struct *info)
{
  volatile UART_REGS                *UartBaseAddr = 
        (UART_REGS *)info->state->iomem_base;

  if(info->MCR & UART_MCR_RTS) {
    if(HWFlowMode)
      UART_SetACIOControl(UartBaseAddr, ACIO_RTS, 3, 4);
    else
#if RS485
      UART_SetACIOControl(UartBaseAddr, ACIO_RTS, 3, 2);  // set RTS
#else
      UART_SetACIOControl(UartBaseAddr, ACIO_RTS, 2, 2);  // set RTS
#endif
  } else
#if RS485
    UART_SetACIOControl(UartBaseAddr, ACIO_RTS, 3, 0);  // clr RTS
#else
    UART_SetACIOControl(UartBaseAddr, ACIO_RTS, 2, 0);  // clr RTS
#endif
}

/*
 * ----------------------------------------------------------------------
 *
 * Here starts the interrupt handling routines.  All of the following
 * subroutines are declared as inline and are folded into
 * uart_interrupt().  They were separated out for readability's sake.
 *
 * Note: uart_interrupt() is a "fast" interrupt, which means that it
 * runs with interrupts turned off.  People who may want to modify
 * uart_interrupt() should try to keep the interrupt handler as fast as
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
static inline void uart_sched_event(struct async_struct *info, int event)
{
  info->event |= 1 << event;
  queue_task(&info->tqueue, &tq_serial);
  mark_bh(SERIAL_BH);
}

static dma_addr_t setup_dma(int channel, int mode, void *buffer, int len)
{
  unsigned long dma_lock;
  dma_addr_t    addr;
  int           dma_direction;

  dma_direction = (mode == DMA_MODE_READ) ? PCI_DMA_FROMDEVICE : PCI_DMA_TODEVICE;
  addr = pci_map_single(NULL, buffer, len, dma_direction);

  // Refer to "Linux Device Drivers", p.417.
  dma_lock = claim_dma_lock();
  disable_dma(channel);
  set_dma_mode(channel, mode);
//	set_dma_data_packing(channel, 1);
  set_dma_addr(channel, addr);
  set_dma_count(channel, len);
  enable_dma(channel);
  release_dma_lock(dma_lock);

  return addr;
}

#if DMA_OUT_OPEN
static inline void dma_out(struct async_struct *info, char *addr, int count)
{
#ifdef SERIAL_DEBUG_INTR
  dbg_out(KERN_INFO "%s - count: %d\n", __FUNCTION__, count);
#endif
  int       channel = (info->state->irq == IRQ_PL_UART)
                        ?  DMA_PL_UART_OUT : DMA_PL_UART_1_OUT;

  memcpy(gDMAWriteBuffer, addr, count);
  setup_dma(channel, DMA_MODE_WRITE, gDMAWriteBuffer, count);
  UART_SetTransmitterData((UART_REGS *)info->state->iomem_base, count, 0);
}
#endif
#if DMA_IN_OPEN
static inline void dma_in(struct async_struct *info, int count)
{
#ifdef SERIAL_DEBUG_INTR
  dbg_out(KERN_INFO "%s - count: %d\n", __FUNCTION__, count);
#endif
  int       channel = (info->state->irq == IRQ_PL_UART)
                        ?  DMA_PL_UART_IN : DMA_PL_UART_1_IN;

  //	enable_irq(DMA_PL_UART_1_IN + 16);		// 16 - DMAC0_IRQ_OFFSET
  setup_dma(channel, DMA_MODE_READ, gDMAReadBuffer, count);

  UART_SetReceiverDMAWaterMark((UART_REGS *)info->state->iomem_base, 
                               DMA_IN_WATERMARK);
}
#endif


static inline void receive_chars(struct async_struct *info, int *status, struct pt_regs *regs)
{
  struct tty_struct   *tty = info->tty;
  struct async_icount *icount;
  int                 max_count = DMA_IN_SIZE;
  ReceiverDMAStatus   ds;
  unsigned int        recCount;

  icount = &info->state->icount;

  *status = 0;

#if DMA_IN_OPEN

  UART_GetReceiverDMAStatus((UART_REGS *)info->state->iomem_base, &ds);
  recCount = ds.RC;

#ifdef SERIAL_DEBUG_INTR
  dbg_out(KERN_INFO "%s - %d bytes received\n", __FUNCTION__, recCount);
#endif

  if(ds.Oe) *status |= UART_LSR_OE;
  if(ds.Fe) *status |= UART_LSR_FE;
  if(ds.Pe) *status |= UART_LSR_PE;
  if(ds.H) {
#ifdef SERIAL_DEBUG_INTR
    dbg_out(KERN_INFO "High Water Mark Exceeded\n");
#endif
    ;
  }

  if(max_count > recCount) max_count = recCount;
#else 
   UCHAR s;

   UART_GetReceiverData((UART_REGS *)info->state->iomem_base, &s);  
#endif

#if DMA_IN_OPEN
  do {
#endif
    if(tty->flip.count >= TTY_FLIPBUF_SIZE) {
      tty->flip.tqueue.routine((void *)tty);
      if(tty->flip.count >= TTY_FLIPBUF_SIZE) return; // if TTY_DONT_FLIP is set
    }

#if DMA_IN_OPEN
    *tty->flip.char_buf_ptr = *(gDMAReadBuffer + (recCount - max_count));
#else
    *tty->flip.char_buf_ptr = s;
#endif

    icount->rx++;

    if(*status & (UART_LSR_BI | UART_LSR_PE | UART_LSR_FE | UART_LSR_OE)) {
      if(*status & UART_LSR_BI) {
        *status &= ~(UART_LSR_FE | UART_LSR_PE);
        icount->brk++;

        if(info->flags & ASYNC_SAK) do_SAK(tty);
      } else if(*status & UART_LSR_PE) icount->parity++;
      else if(*status & UART_LSR_FE)
        icount->frame++;
      if(*status & UART_LSR_OE) icount->overrun++;

      *status &= info->read_status_mask;

      if(*status & (UART_LSR_BI)) {
#ifdef SERIAL_DEBUG_INTR
        dbg_out(KERN_INFO "%s - handling break\n", __FUNCTION__);
#endif
        *tty->flip.flag_buf_ptr = TTY_BREAK;
      } else if(*status & UART_LSR_PE) *tty->flip.flag_buf_ptr = TTY_PARITY;
      else if(*status & UART_LSR_FE)
        *tty->flip.flag_buf_ptr = TTY_FRAME;
    }

    if((*status & info->ignore_status_mask) == 0) {
      tty->flip.flag_buf_ptr++;
      tty->flip.char_buf_ptr++;
      tty->flip.count++;
    }

    if((*status & UART_LSR_OE) && (tty->flip.count < TTY_FLIPBUF_SIZE)) {
#ifdef SERIAL_DEBUG_INTR
      dbg_out(KERN_ERR "Overrun ERROR - status: 0x%02X\n", *status);
#endif
      *tty->flip.flag_buf_ptr = TTY_OVERRUN;

      tty->flip.count++;
      tty->flip.flag_buf_ptr++;
      tty->flip.char_buf_ptr++;
    }
#if DMA_IN_OPEN
  } while(--max_count > 0);

  dma_in(info, DMA_IN_SIZE);
#endif

#if (LINUX_VERSION_CODE > 131394)											/* 2.1.66 */
  tty_flip_buffer_push(tty);
#else
  queue_task_irq_off(&tty->flip.tqueue, &tq_timer);
#endif

#ifdef SERIAL_DEBUG_INTR
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

static inline void transmit_chars(struct async_struct *info, int *intr_done)
{
  int count; 
#if DMA_OUT_OPEN
  int dmaOutCnt = 0;
#else
  UCHAR  status = 0;
#endif

#ifdef SERIAL_DEBUG_WRITE
  dbg_out(KERN_INFO "%s - tts/s%d, head=%d, tail=%d\n", __FUNCTION__, info->line, info->xmit.head, info->xmit.tail);
#endif

  GetTransmitterDataCountReg((UART_REGS *)info->state->iomem_base);
  GetTransmitterCommandStatusReg((UART_REGS *)info->state->iomem_base);

  if(info->x_char) {
#if DMA_OUT_OPEN
    dma_out(info, (char *) &(info->x_char), 1);
#else
    UART_GetTransmitterStatus((UART_REGS *)info->state->iomem_base, &status);
    if (1 == status)  return;
    UART_SetTransmitterData((UART_REGS *)info->state->iomem_base, 0, info->x_char);
#endif
    info->state->icount.tx++;
    info->x_char = 0;
    if(intr_done) *intr_done = 0;
    return;
  }

  if(info->xmit.head == info->xmit.tail || info->tty->stopped || info->tty->hw_stopped) {
    info->IER &= ~UART_IER_THRI;
    return;
  }

#if DMA_OUT_OPEN
  count = MAX_DMA_OUT_COUNT;
  do {
    *(gTmpBuffer + dmaOutCnt++) = info->xmit.buf[info->xmit.tail];

    info->xmit.tail = (info->xmit.tail + 1) & (SERIAL_XMIT_SIZE - 1);
    info->state->icount.tx++;
    if(info->xmit.head == info->xmit.tail) break;
  } while(--count > 0);

  dma_out(info, gTmpBuffer, dmaOutCnt);
#else
    count = info->xmit_fifo_size;
    do {
	UART_GetTransmitterStatus((UART_REGS *)info->state->iomem_base, &status);
        if (1 == status) break;
        UART_SetTransmitterData((UART_REGS *)info->state->iomem_base, 0, info->xmit.buf[info->xmit.tail]);
        info->xmit.tail = (info->xmit.tail + 1) & (SERIAL_XMIT_SIZE-1);
        info->state->icount.tx++;		
        if (info->xmit.head == info->xmit.tail)
            break;
    } while (--count > 0);
#endif

  if(CIRC_CNT(info->xmit.head, info->xmit.tail, SERIAL_XMIT_SIZE) < WAKEUP_CHARS)
    uart_sched_event(info, RS_EVENT_WRITE_WAKEUP);

  if(intr_done) *intr_done = 0;
  if(info->xmit.head == info->xmit.tail) info->IER &= ~UART_IER_THRI;

#ifdef SERIAL_DEBUG_WRITE
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

/*
 ** ------------------------------------------------------------
 ** uart_stop() and uart_start()
 **
 ** This routines are called before setting or resetting tty->stopped.
 ** They enable or disable transmitter interrupts, as necessary.
 ** ------------------------------------------------------------
 **/
static void uart_stop(struct tty_struct *tty)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  unsigned long       flags;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d, count = %d\n", __FUNCTION__, info->line, info->state->count);
#endif

  if(serial_paranoia_check(info, tty->device, "uart_stop")) return;
  save_flags_cli(flags);
  if(info->IER & UART_IER_THRI) {
    info->IER &= ~UART_IER_THRI;
  }

  restore_flags(flags);

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

static void uart_start(struct tty_struct *tty)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  unsigned long       flags;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d, count = %d\n", __FUNCTION__, info->line, info->state->count);
#endif

  if(serial_paranoia_check(info, tty->device, "uart_start")) return;
  save_flags_cli(flags);
  if(info->xmit.head != info->xmit.tail && info->xmit.buf && !(info->IER & UART_IER_THRI)) {
    info->IER |= UART_IER_THRI;
    transmit_chars(info, 0);
  }

  restore_flags(flags);

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

static inline void check_modem_status(struct async_struct *info)
{
  int                 status;
  struct async_icount *icount;

  //	status = PL_UARTReadByte(PL_UART_MSR);	//serial_in(info, UART_MSR);
  if(status & UART_MSR_ANY_DELTA) {
    icount = &info->state->icount;

    /* update input line counters */
    if(status & UART_MSR_TERI) icount->rng++;
    if(status & UART_MSR_DDSR) icount->dsr++;
    if(status & UART_MSR_DDCD) {
      icount->dcd++;
    }

    if(status & UART_MSR_DCTS) icount->cts++;
    wake_up_interruptible(&info->delta_msr_wait);
  }

  if((info->flags & ASYNC_CHECK_CD) && (status & UART_MSR_DDCD)) {
#if (defined(SERIAL_DEBUG_OPEN) || defined(SERIAL_DEBUG_INTR))
    dbg_out("ttys%d CD now %s...\n", info->line, (status & UART_MSR_DCD) ? "on" : "off");
#endif
    if(status & UART_MSR_DCD) {
      wake_up_interruptible(&info->open_wait);
    } else if(!((info->flags & ASYNC_CALLOUT_ACTIVE) && (info->flags & ASYNC_CALLOUT_NOHUP))) {
#ifdef SERIAL_DEBUG_OPEN
      dbg_out("doing serial hangup...\n");
#endif
      if(info->tty) tty_hangup(info->tty);
    }
  }

  if(info->flags & ASYNC_CTS_FLOW) {
    if(info->tty->hw_stopped) {
      if(status & UART_MSR_CTS) {
#if (defined(SERIAL_DEBUG_INTR) || defined(SERIAL_DEBUG_FLOW))
        dbg_out("CTS tx start...\n");
#endif
        info->tty->hw_stopped = 0;
        info->IER |= UART_IER_THRI;

        //				PL_UARTWriteByte(info->IER, PL_UART_IER);
        //serial_out(info, UART_IER, info->IER);
        uart_sched_event(info, RS_EVENT_WRITE_WAKEUP);
        return;
      }
    } else {
      if(!(status & UART_MSR_CTS)) {
#if (defined(SERIAL_DEBUG_INTR) || defined(SERIAL_DEBUG_FLOW))
        dbg_out("CTS tx stop...\n");
#endif
        info->tty->hw_stopped = 1;
        info->IER &= ~UART_IER_THRI;

        //				PL_UARTWriteByte(info->IER, PL_UART_IER);
        //serial_out(info, UART_IER, info->IER);
      }
    }
  }
}

/*
 * This is the serial driver's interrupt routine for a single port
 */
static void uart_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
  int                 status;
  int                 pass_counter = 0;
  struct async_struct *info;
  InterruptStatus     is;

#ifdef SERIAL_DEBUG_INTR
  dbg_out("=====> Enter Interrupt - irq=%d\n", irq);
#endif

  info = IRQ_ports[irq];

  if(!info || !info->tty) return;
  if((info->state->type != PORT_PL_UART) &&
     (info->state->type != PORT_PL_UART_1))
    return;

  do {
    UART_GetInterruptStatus((UART_REGS *)info->state->iomem_base, &is);

#ifdef SERIAL_DEBUG_INTR
    dbg_out("%s - Ti=%d, Ri=%d\n", __FUNCTION__, is.Ti, is.Ri);
#endif

    if(is.Ti) // Transmitter Interrupt Detected
      transmit_chars(info, 0);
    if(is.Ri) // Receiver Interrupt Detected
      receive_chars(info, &status, regs);
    if(pass_counter++ > RS_ISR_PASS_LIMIT) break;
} while(is.Ti || is.Ri);

  info->last_active = jiffies;
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
 * uart_interrupt() has returned, BUT WITH INTERRUPTS TURNED ON.  This
 * is where time-consuming activities which can not be done in the
 * interrupt driver proper are done; the interrupt driver schedules
 * them using uart_sched_event(), and they get done here.
 */
#if 0
static void do_serial_bh(void)
{
  udelay(500);  // note this
  run_task_queue(&tq_serial);
}
#endif

static void do_softint(void *private_)
{
  struct async_struct *info = (struct async_struct *)private_;
  struct tty_struct   *tty;

  tty = info->tty;
  if(!tty) return;

  if(test_and_clear_bit(RS_EVENT_WRITE_WAKEUP, &info->event)) {
    if((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) && tty->ldisc.write_wakeup) (tty->ldisc.write_wakeup) (tty);
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
static void uart_timer(unsigned long dummy)
{
  static unsigned long  last_strobe;
  struct async_struct   *info;
  unsigned int          i;
  unsigned long         flags;

  if((jiffies - last_strobe) >= RS_STROBE_TIME) {
    for(i = 0; i < NR_IRQS; i++) {
      info = IRQ_ports[i];
      if(!info) continue;
      save_flags_cli(flags);

      //			uart_interrupt(i, NULL, NULL);
      restore_flags(flags);
    }
  }

  last_strobe = jiffies;
  mod_timer(&serial_timer, jiffies + RS_STROBE_TIME);

  if(IRQ_ports[0]) {
    save_flags_cli(flags);

    //		uart_interrupt(0, NULL, NULL);
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
  struct async_struct *info;
  int                 timeout = 60 * HZ;          /* 60 seconds === a long time :-) */

  info = IRQ_ports[irq];
  if(!info) {
    IRQ_timeout[irq] = 60 * HZ;
    return;
  }

  while(info) {
    if(info->timeout < timeout) timeout = info->timeout;
    info = info->next_port;
  }

  if(!irq) timeout = timeout / 2;
  IRQ_timeout[irq] = (timeout > 3) ? timeout - 2 : 1;
}

#if 0
static void init_gpio(void)
{
  int input_pins = PIN_DSR | PIN_DCD | PIN_RI;   // 5-DSR, 6-DCD, 7-RI

  /* Software reset PWM */
  writew(0, GPIO_RSTN);
  writew(RSTN_PWM, GPIO_RSTN);
  writew(0, GPIO_DO);                         // clear all GPIO output
  writew(0, GPIO_ENB);                        // disable all GPIO

  /* config input GPIO */
  writew(~input_pins & 0xFFFF, GPIO_OE);      /* config input pin */
  writew(0, GPIO_PF0_PU);                     /* disable pull up */
  writew(input_pins, GPIO_PF0_PD);            /* enable pull down for input pin */

  writew(input_pins, GPIO_KEY_ENB);
  writew(~input_pins, GPIO_KEY_POL);

  /* Software reset GPIO KEY */
  writew(RSTN_PWM | RSTN_KEY, GPIO_RSTN);
}
#endif

static int startup(struct async_struct *info)
{
  unsigned long flags;
  int           retval = 0;
  void (*handler) (int, void *, struct pt_regs *);

  InterruptControl    ic = {0};

  struct serial_state *state = info->state;
  unsigned long       page;

  page = get_zeroed_page(GFP_KERNEL);
  if(!page) return -ENOMEM;

  save_flags_cli(flags);

  if(info->flags & ASYNC_INITIALIZED) {
    free_page(page);
    goto errout;
  }

  if(!CONFIGURED_SERIAL_PORT(state) || !state->type) {
    if(info->tty) set_bit(TTY_IO_ERROR, &info->tty->flags);
    free_page(page);
    goto errout;
  }

  if(info->xmit.buf)
    free_page(page);
  else
    info->xmit.buf = (unsigned char *)page;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d (irq %d)\n", __FUNCTION__, info->line, state->irq);
#endif

  if(uart_config[state->type].flags & UART_STARTECH) {

    /* Wake up UART */
    /* do nothing for PL1061 */
  }

  if(state->type == PORT_PL_SIO) {

    /* Wake up and initialize UART */
    /* do nothing for PL1061 SIO */
  }
/* 
  else if(state->type == PORT_PL_UART_1) {
    ;
  }
*/

  /*
   * Clear the FIFO buffers and disable them
   * (they will be reenabled in change_speed())
   */
  if(uart_config[state->type].flags & UART_CLEAR_FIFO) {

    /* do nothing for PL1061 SIO */
  }

  /*
   * At this point there's no way the LSR could still be 0xFF;
   * if it is, then bail out, because there's likely no UART
   * here.
   */
 volatile UART_REGS *UartBaseAddr = (UART_REGS *)info->state->iomem_base;

  UART_SetTransmitterCommand(UartBaseAddr, 0);    // reset transmitter
#if DMA_OUT_OPEN
  UART_SetTransmitterCommand(UartBaseAddr, 0xC);  // DMA Data Out Mode
#else
  UART_SetTransmitterCommand(UartBaseAddr, 0x8);  // Programmed Data Out Mode
#endif
  UART_SetReceiverCommand(UartBaseAddr, 0);       // reset receiver
#if DMA_IN_OPEN
  UART_SetReceiverCommand(UartBaseAddr, 0xC);     // DMA Data In Mode
#else
  UART_SetReceiverCommand(UartBaseAddr, 0x08);     // Programmed Data In Mode
#endif

#if DMA_OUT_OPEN
  gDMAWriteBuffer = (unsigned char *)__get_dma_pages(GFP_KERNEL, DMA_OUT_BUF_SIZE / PAGE_SIZE - 1);
  if(!gDMAWriteBuffer) {
    printk(KERN_ERR "startup - no memory, gDMAWriteBuffer failed\n");
    return -ENOMEM;
  }
#endif

#if DMA_IN_OPEN
  gDMAReadBuffer = (unsigned char *)__get_dma_pages(GFP_KERNEL, DMA_IN_BUF_SIZE / PAGE_SIZE - 1);
  if(!gDMAReadBuffer) {
    printk(KERN_ERR "startup - no memory, gDMAReadBuffer failed\n");
    return -ENOMEM;
  }
#endif

  gTmpBuffer = kmalloc(PAGE_SIZE, GFP_KERNEL);
  if(!gTmpBuffer) {
    printk(KERN_ERR "startup - no memory, gTmpBuffer failed\n");
    return -ENOMEM;
  }

#if 1 // May it should be disabled.
  info->IER = UART_IER_MSI | UART_IER_RLSI | UART_IER_RDI;
  ic.T = ic.E = ic.R = 1;
  ic.RIDL = 10;       // note this
  UART_SetInterruptControl(UartBaseAddr, &ic);
#endif

  /*
   * Allocate the IRQ if necessary
   */
  if(state->irq && (!IRQ_ports[state->irq] || !IRQ_ports[state->irq]->next_port)) {
    if(IRQ_ports[state->irq]) {
      retval = -EBUSY;
      goto errout;
    } else
      handler = uart_interrupt;

    retval = request_irq(state->irq, handler, 0, "PLuart", &IRQ_ports[state->irq]);

#ifdef SERIAL_DEBUG_OPEN
    dbg_out(KERN_INFO "%s - state->irq=%d, retval=%d\n", __FUNCTION__, state->irq, retval);
#endif

    if(retval) {
      if(capable(CAP_SYS_ADMIN)) {
        if(info->tty) set_bit(TTY_IO_ERROR, &info->tty->flags);
        retval = 0;
      }

      goto errout;
    }
  }

  if (info->state->irq == IRQ_PL_UART) {
#if DMA_OUT_OPEN
    if(request_dma(DMA_PL_UART_OUT, "uart_dma_out")) 
        printk(KERN_ERR "init_uart - request_uart_dma_out failed\n");
#endif

#if DMA_IN_OPEN
    if(request_dma(DMA_PL_UART_IN, "uart_dma_in")) 
        printk(KERN_ERR "init_uart - request_uart_dma_in failed\n");
#endif
  }
  else {
#if DMA_OUT_OPEN
    if(request_dma(DMA_PL_UART_1_OUT, "uart_dma_out")) 
        printk(KERN_ERR "init_uart - request_uart_dma_out failed\n");
#endif

#if DMA_IN_OPEN
    if(request_dma(DMA_PL_UART_1_IN, "uart_dma_in")) 
        printk(KERN_ERR "init_uart - request_uart_dma_in failed\n");
#endif
  }

#if DMA_IN_OPEN
  dma_in(info, DMA_IN_SIZE);
#endif

  //init_gpio();

  /*
   * Insert serial port into IRQ chain.
   */
  info->prev_port = 0;
  info->next_port = IRQ_ports[state->irq];
  if(info->next_port) info->next_port->prev_port = info;
  IRQ_ports[state->irq] = info;
  figure_IRQ_timeout(state->irq);

  /*
   * Now, initialize the UART
   */
  //	PL_UARTWriteByte(UART_LCR_WLEN8, PL_UART_LCR);
  //	serial_outp(info, UART_LCR, UART_LCR_WLEN8);	/* reset DLAB */
  info->MCR = 0;
  if(info->tty->termios->c_cflag & CBAUD) info->MCR = UART_MCR_DTR | UART_MCR_RTS;
  {
    if(state->irq != 0) info->MCR |= UART_MCR_OUT2;
  }

  info->MCR |= ALPHA_KLUDGE_MCR;    /* Don't ask */

  ProcessDTR(info);
  ProcessRTS(info);

  if(info->tty) clear_bit(TTY_IO_ERROR, &info->tty->flags);
  info->xmit.head = info->xmit.tail = 0;

  /*
   * Set up serial timers...
   */
  mod_timer(&serial_timer, jiffies + 2 * HZ / 100);

  /*
   * Set up the tty->alt_speed kludge
   */
#if (LINUX_VERSION_CODE >= 131394)	/* Linux 2.1.66 */
  if(info->tty) {
    if((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_HI) info->tty->alt_speed = 57600;
    if((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_VHI) info->tty->alt_speed = 115200;
    if((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_SHI) info->tty->alt_speed = 230400;
    if((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_WARP) info->tty->alt_speed = 460800;
  }
#endif

  /*
   * and set the speed of the serial port
   */
  change_speed(info, 0);

  info->flags |= ASYNC_INITIALIZED;
  restore_flags(flags);

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif

  return 0;

  errout:
  restore_flags(flags);

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit failed, retval=%d\n", __FUNCTION__, retval);
#endif

  return retval;
}

/*
 * This routine will shutdown a serial port; interrupts are disabled, and
 * DTR is dropped if the hangup on close termio flag is on.
 */
static void shutdown(struct async_struct *info)
{
  unsigned long       flags;
  struct serial_state *state;
  int                 retval;

  if(!(info->flags & ASYNC_INITIALIZED)) return;

  state = info->state;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d (irq %d)....\n", __FUNCTION__, info->line, state->irq);
#endif

  save_flags_cli(flags);            /* Disable interrupts */

  /*
   * clear delta_msr_wait queue to avoid mem leaks: we may free the irq
   * here so the queue might never be waken up
   */
  wake_up_interruptible(&info->delta_msr_wait);

  /*
   * First unlink the serial port from the IRQ chain...
   */
  if(info->next_port) info->next_port->prev_port = info->prev_port;
  if(info->prev_port)
    info->prev_port->next_port = info->next_port;
  else
    IRQ_ports[state->irq] = info->next_port;
  figure_IRQ_timeout(state->irq);

  if (info->state->irq == IRQ_PL_UART) {
#if DMA_OUT_OPEN
    free_dma(DMA_PL_UART_OUT);
#endif
#if DMA_IN_OPEN
    free_dma(DMA_PL_UART_IN);
#endif
  }
  else {
    free_dma(DMA_PL_UART_1_OUT);
#if DMA_IN_OPEN
    free_dma(DMA_PL_UART_1_IN);
#endif
  }

  /*
   * Free the IRQ, if necessary
   */
  if(state->irq && (!IRQ_ports[state->irq] || !IRQ_ports[state->irq]->next_port)) {
    if(IRQ_ports[state->irq]) {
      free_irq(state->irq, &IRQ_ports[state->irq]);
      retval = request_irq(state->irq, uart_interrupt, 0, "PLuart", &IRQ_ports[state->irq]);
      if(retval) {
        dbg_out(KERN_ERR "%s - request_irq error %d\nCouldn't reacquire IRQ.\n", __FUNCTION__, retval);
      }
    } else
      free_irq(state->irq, &IRQ_ports[state->irq]);
  }

  if(info->xmit.buf) {
    unsigned long pg = (unsigned long)info->xmit.buf;
    info->xmit.buf = 0;
    free_page(pg);
  }

  info->IER = 0;

  //	PL_UARTWriteByte(0x00, PL_UART_IER);
  //serial_outp(info, UART_IER, 0x00);	/* disable all intrs */
  info->MCR &= ~UART_MCR_OUT2;
  info->MCR |= ALPHA_KLUDGE_MCR;  /* Don't ask */

  /* disable break condition */
  //	PL_UARTWriteByte(PL_UARTReadByte(PL_UART_LCR) &~UART_LCR_SBC, PL_UART_LCR);
  //serial_out(info, UART_LCR, serial_inp(info, UART_LCR) & ~UART_LCR_SBC);
  if(!info->tty || (info->tty->termios->c_cflag & HUPCL)) info->MCR &= ~(UART_MCR_DTR | UART_MCR_RTS);

  ProcessDTR(info);
  ProcessRTS(info);

  if(info->tty) set_bit(TTY_IO_ERROR, &info->tty->flags);

  if(uart_config[info->state->type].flags & UART_STARTECH) {

    /* Arrange to enter sleep mode */
  }

  if(info->state->type == PORT_16750) {

    /* Arrange to enter sleep mode */
  }

  info->flags &= ~ASYNC_INITIALIZED;
  restore_flags(flags);
}

#if (LINUX_VERSION_CODE < 131394) /* Linux 2.1.66 */
static int  baud_table[] =
{
  0,
  50,
  75,
  110,
  134,
  150,
  200,
  300,
  600,
  1200,
  1800,
  2400,
  4800,
  9600,
  19200,
  38400,
  57600,
  115200,
  230400,
  460800,
  0
};

static int tty_get_baud_rate(struct tty_struct *tty)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  unsigned  int        cflag, i;

  cflag = tty->termios->c_cflag;

  i = cflag & CBAUD;
  if(i & CBAUDEX) {
    i &= ~CBAUDEX;
    if(i < 1 || i > 2)
      tty->termios->c_cflag &= ~CBAUDEX;
    else
      i += 15;
  }

  if(i == 15) {
    if((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_HI) i += 1;
    if((info->flags & ASYNC_SPD_MASK) == ASYNC_SPD_VHI) i += 2;
  }

  return baud_table[i];
}
#endif

#define RELEVANT_IFLAG(iflag) (iflag & (IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK))

/*
 * This routine is called to set the UART divisor registers to match
 * the specified baud rate for a serial port.
 */
static void change_speed(struct async_struct *info, struct termios *old_termios)
{
  int             quot = 0, baud_base, baud;
  unsigned        cflag, cval;
  int             bits;
  unsigned long   flags;
  LineCodingStru  lc = {0};

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d, count = %d\n", __FUNCTION__, info->line, info->state->count);
#endif

  if(!info->tty || !info->tty->termios) return;
  cflag = info->tty->termios->c_cflag;
  if(!CONFIGURED_SERIAL_PORT(info)) return;

  /* byte size and parity */
  switch(cflag & CSIZE) {
    case CS5: cval = 0x00; bits = 7; lc.DSZ = 0; break;
    case CS6: cval = 0x01; bits = 8; lc.DSZ = 1; break;
    case CS7: cval = 0x02; bits = 9; lc.DSZ = 2; break;
    case CS8: cval = 0x03; bits = 10; lc.DSZ = 3; break;
      // Never happens, but GCC is too dumb to figure it out
    default:  cval = 0x00; bits = 7; lc.DSZ = 0; break;
  }

  if(cflag & CSTOPB) {
    cval |= 0x04;
    bits++;
    lc.STP = 3;
  }

  if(cflag & PARENB) {
    cval |= UART_LCR_PARITY;
    bits++;
    lc.PAR = 5;                     // odd (assume)
  }

  if(!(cflag & PARODD)) {
    cval |= UART_LCR_EPAR;
    if(cflag & PARENB) lc.PAR = 6;  // even
  }

#ifdef CMSPAR
  if(cflag & CMSPAR) cval |= UART_LCR_SPAR;
#endif

  /* Determine divisor based on baud rate */
  baud = tty_get_baud_rate(info->tty);
  if(!baud) baud = 9600;            /* B0 transition handled in rs_set_termios */
  baud_base = info->state->baud_base;

  volatile UART_REGS    *UartBaseAddr = (UART_REGS *)info->state->iomem_base;
  if(info->state->type == PORT_PL_UART ||
     info->state->type == PORT_PL_UART_1)
        UART_SetBaudRate(UartBaseAddr, baud, 0);

  /* If the quotient is zero refuse the change */
  /* As a last resort, if the quotient is zero, default to 9600 bps */
  /*
   * Work around a bug in the Oxford Semiconductor 952 rev B
   * chip which causes it to seriously miscalculate baud rates
   * when DLL is 0.
   */
  info->quot = quot;
  info->timeout = ((info->xmit_fifo_size * HZ * bits * quot) / baud_base);
  info->timeout += HZ / 50;         /* Add .02 seconds of slop */

  /* Set up FIFO's */
  /* CTS flow control flag and modem status interrupts */
  info->IER &= ~UART_IER_MSI;
  if(info->flags & ASYNC_HARDPPS_CD) info->IER |= UART_IER_MSI;
  if(cflag & CRTSCTS) {
    info->flags |= ASYNC_CTS_FLOW;
    info->IER |= UART_IER_MSI;
    lc.MS = 2;
    HWFlowMode = 1;
    UART_SetACIOControl(UartBaseAddr, ACIO_RTS, 3, 4);
    UART_SetACIOControl(UartBaseAddr, ACIO_CTS, 0, 0);
  } else {
    info->flags &= ~ASYNC_CTS_FLOW;
    lc.MS = 0;
    HWFlowMode = 0;
#if RS485
    UART_SetACIOControl(UartBaseAddr, ACIO_RTS, 3, 2);
#else
    UART_SetACIOControl(UartBaseAddr, ACIO_RTS, 2, 2);
#endif
  }

  if(cflag & CLOCAL)
    info->flags &= ~ASYNC_CHECK_CD;
  else {
    info->flags |= ASYNC_CHECK_CD;
    info->IER |= UART_IER_MSI;
  }

/*
   * Set up parity check flag
   */
#define RELEVANT_IFLAG(iflag) (iflag & (IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK))
  info->read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
  if(I_INPCK(info->tty)) info->read_status_mask |= UART_LSR_FE | UART_LSR_PE;
  if(I_BRKINT(info->tty) || I_PARMRK(info->tty)) info->read_status_mask |= UART_LSR_BI;

  /*
   * Characters to ignore
   */
  info->ignore_status_mask = 0;
  if(I_IGNPAR(info->tty)) info->ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
  if(I_IGNBRK(info->tty)) {
    info->ignore_status_mask |= UART_LSR_BI;

    /*
     * If we're ignore parity and break indicators, ignore
     * overruns too.  (For real raw support).
     */
    if(I_IGNPAR(info->tty)) info->ignore_status_mask |= UART_LSR_OE;
  }

  /*
   * !!! ignore all characters if CREAD is not set
   */
  if((cflag & CREAD) == 0) info->ignore_status_mask |= UART_LSR_DR;
  save_flags_cli(flags);

  UART_SetLineCoding(UartBaseAddr, &lc);

  info->LCR = cval; /* Save LCR */
  restore_flags(flags);

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

static void uart_put_char(struct tty_struct *tty, unsigned char ch)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  unsigned long       flags;

#ifdef SERIAL_DEBUG_WRITE
  dbg_out(KERN_INFO "%s - tts/s%d, ch=0x%02X\n", __FUNCTION__, info->line, ch);
#endif

  if(serial_paranoia_check(info, tty->device, "uart_put_char")) return;
  if(!tty || !info->xmit.buf) return;

  save_flags_cli(flags);
  if(CIRC_SPACE(info->xmit.head, info->xmit.tail, SERIAL_XMIT_SIZE) == 0) {
    restore_flags(flags);
    return;
  }

  info->xmit.buf[info->xmit.head] = ch;
  info->xmit.head = (info->xmit.head + 1) & (SERIAL_XMIT_SIZE - 1);
  restore_flags(flags);

#ifdef SERIAL_DEBUG_WRITE
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

static void uart_flush_chars(struct tty_struct *tty)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  unsigned long       flags;
  InterruptControl    ic = {0};

#ifdef SERIAL_DEBUG_WRITE
  dbg_out(KERN_INFO "%s - tts/s%d\n", __FUNCTION__, info->line);
#endif

  if(serial_paranoia_check(info, tty->device, "uart_flush_chars")) return;
  if(info->xmit.head == info->xmit.tail || tty->stopped || tty->hw_stopped || !info->xmit.buf) return;

  save_flags_cli(flags);
  info->IER |= UART_IER_THRI;

  ic.E = ic.T = 1;
  UART_SetInterruptControl((UART_REGS *)info->state->iomem_base, &ic);

  restore_flags(flags);

#ifdef SERIAL_DEBUG_WRITE
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

static int uart_write(struct tty_struct *tty, int from_user, const unsigned char *buf, int count)
{
  int                   c, ret = 0;
  struct async_struct   *info = (struct async_struct *)tty->driver_data;
  unsigned long         flags;
#if DMA_OUT_OPEN
  TransmitterDMAStatus  tds;
#endif

#ifdef SERIAL_DEBUG_WRITE
  dbg_out(KERN_INFO "%s - tts/s%d, from user=%d data count=%d\n", __FUNCTION__, info->line, from_user, count);
#endif

  if(serial_paranoia_check(info, tty->device, "uart_write")) return 0;
  if(!tty || !info->xmit.buf || !tmp_buf) return 0;

  save_flags(flags);

  if(from_user) {
    down(&tmp_buf_sem);
    while(1) {
      int c1;
      c = CIRC_SPACE_TO_END(info->xmit.head, info->xmit.tail, SERIAL_XMIT_SIZE);

      if(count < c) c = count;
      if(c <= 0) break;
      c -= copy_from_user(tmp_buf, buf, c);

      if(!c) {
        if(!ret) ret = -EFAULT;
        break;
      }

      cli();

      c1 = CIRC_SPACE_TO_END(info->xmit.head, info->xmit.tail, SERIAL_XMIT_SIZE);
      if(c1 < c) c = c1;
      memcpy(info->xmit.buf + info->xmit.head, tmp_buf, c);

      info->xmit.head = ((info->xmit.head + c) & (SERIAL_XMIT_SIZE - 1));
      restore_flags(flags);
      buf += c;
      count -= c;
      ret += c;
    }

    up(&tmp_buf_sem);
  } else {
    cli();

    while(1) {
      c = CIRC_SPACE_TO_END(info->xmit.head, info->xmit.tail, SERIAL_XMIT_SIZE);
      if(count < c) c = count;
      if(c <= 0) {
        break;
      }

      memcpy(info->xmit.buf + info->xmit.head, buf, c);

      info->xmit.head = ((info->xmit.head + c) & (SERIAL_XMIT_SIZE - 1));
      buf += c;
      count -= c;
      ret += c;
    }

    restore_flags(flags);
  }

  if(info->xmit.head != info->xmit.tail && !tty->stopped && !tty->hw_stopped && !(info->IER & UART_IER_THRI)) {
    info->IER |= UART_IER_THRI;

#if DMA_OUT_OPEN
    UART_GetTransmitterDMAStatus((UART_REGS *)info->state->iomem_base, &tds);
    if(tds.ST != 1) {
      dma_out(info, (char *) &(info->xmit.buf[info->xmit.tail]), 1);
      info->xmit.tail = (info->xmit.tail + 1) & (SERIAL_XMIT_SIZE - 1);
      info->state->icount.tx++;
    }
#else
     transmit_chars(info, 0);
#endif
  }

#ifdef SERIAL_DEBUG_WRITE
  dbg_out(KERN_INFO "%s - exit successfully, ret=%d\n", __FUNCTION__, ret);
#endif

  return ret;
}

static int uart_write_room(struct tty_struct *tty)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;

#ifdef SERIAL_DEBUG_WRITE
  dbg_out(KERN_INFO "%s - tts/s%d\n", __FUNCTION__, info->line);
#endif

  if(serial_paranoia_check(info, tty->device, "uart_write_room")) return 0;
  return CIRC_SPACE(info->xmit.head, info->xmit.tail, SERIAL_XMIT_SIZE);
}

static int uart_chars_in_buffer(struct tty_struct *tty)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;

#ifdef SERIAL_DEBUG_WRITE
  dbg_out(KERN_INFO "%s - tts/s%d\n", __FUNCTION__, info->line);
#endif

  if(serial_paranoia_check(info, tty->device, "uart_chars_in_buffer")) return 0;
  return CIRC_CNT(info->xmit.head, info->xmit.tail, SERIAL_XMIT_SIZE);
}

static void uart_flush_buffer(struct tty_struct *tty)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  unsigned long       flags;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d\n", __FUNCTION__, info->line);
#endif

  if(serial_paranoia_check(info, tty->device, "uart_flush_buffer")) return;

  save_flags_cli(flags);
  info->xmit.head = info->xmit.tail = 0;
  restore_flags(flags);
  wake_up_interruptible(&tty->write_wait);
#ifdef SERIAL_HAVE_POLL_WAIT
  wake_up_interruptible(&tty->poll_wait);
#endif
  if((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) && tty->ldisc.write_wakeup) {
    (tty->ldisc.write_wakeup) (tty);
  }

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

/*
 * This function is used to send a high-priority XON/XOFF character to
 * the device
 */
static void uart_send_xchar(struct tty_struct *tty, char ch)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;

#ifdef SERIAL_DEBUG_WRITE
  dbg_out(KERN_INFO "%s - tts/s%d ch=0x%02X\n", __FUNCTION__, info->line, ch);
#endif

  if(serial_paranoia_check(info, tty->device, "uart_send_xchar")) return;

  info->x_char = ch;
  if(ch) {

    /* Make sure transmit interrupts are on */
    info->IER |= UART_IER_THRI;

    UART_SetTransmitterData((UART_REGS *)info->state->iomem_base, 0, ch);
  }

#ifdef SERIAL_DEBUG_WRITE
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

/*
 * ------------------------------------------------------------
 * uart_throttle()
 *
 * This routine is called by the upper-layer tty layer to signal that
 * incoming characteuart should be throttled.
 * ------------------------------------------------------------
 */
static void uart_throttle(struct tty_struct *tty)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  unsigned long       flags;

#ifdef SERIAL_DEBUG_THROTTLE
  char                buf[64];
  dbg_out(KERN_INFO "%s %s: %d\n", __FUNCTION__, tty_name(tty, buf), tty->ldisc.chars_in_buffer(tty));
#endif

  if(serial_paranoia_check(info, tty->device, "uart_throttle")) return;
  if(I_IXOFF(tty)) uart_send_xchar(tty, STOP_CHAR(tty));
  if(tty->termios->c_cflag & CRTSCTS) info->MCR &= ~UART_MCR_RTS;

  save_flags_cli(flags);
  ProcessRTS(info);
  restore_flags(flags);
}

static void uart_unthrottle(struct tty_struct *tty)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  unsigned long       flags;

#ifdef SERIAL_DEBUG_THROTTLE
  char                buf[64];
  dbg_out(KERN_INFO "%s %s: %d\n", __FUNCTION__, tty_name(tty, buf), tty->ldisc.chars_in_buffer(tty));
#endif

  if(serial_paranoia_check(info, tty->device, "uart_unthrottle")) return;
  if(I_IXOFF(tty)) {
    if(info->x_char)
      info->x_char = 0;
    else
      uart_send_xchar(tty, START_CHAR(tty));
  }

  if(tty->termios->c_cflag & CRTSCTS) info->MCR |= UART_MCR_RTS;
  save_flags_cli(flags);
  ProcessRTS(info);
  restore_flags(flags);
}

/*
 * ------------------------------------------------------------
 * uart_ioctl() and friends
 * ------------------------------------------------------------
 */
static int get_serial_info(struct async_struct *info, struct serial_struct *retinfo)
{
  struct serial_struct  tmp;
  struct serial_state   *state = info->state;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d, count = %d\n", __FUNCTION__, info->line, info->state->count);
#endif

  if(!retinfo) return -EFAULT;
  memset(&tmp, 0, sizeof(tmp));
  tmp.type = state->type;
  tmp.line = state->line;
  tmp.port = state->port;
  if(HIGH_BITS_OFFSET)
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
  if(copy_to_user(retinfo, &tmp, sizeof(*retinfo))) return -EFAULT;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif

  return 0;
}

static int set_serial_info(struct async_struct *info, struct serial_struct *new_info)
{
  struct serial_struct  new_serial;
  struct serial_state   old_state, *state;
  unsigned int          i, change_irq, change_port;
  int                   retval = 0;
  unsigned long         new_port;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d, count = %d\n", __FUNCTION__, info->line, info->state->count);
#endif

  if(copy_from_user(&new_serial, new_info, sizeof(new_serial))) return -EFAULT;
  state = info->state;
  old_state = *state;

  new_port = new_serial.port;
  if(HIGH_BITS_OFFSET) new_port += (unsigned long)new_serial.port_high << HIGH_BITS_OFFSET;

  change_irq = new_serial.irq != state->irq;
  change_port = (new_port != ((int)state->port)) || (new_serial.hub6 != state->hub6);

  if(!capable(CAP_SYS_ADMIN)) {
    if
      (
      change_irq ||
      change_port ||
      (new_serial.baud_base != state->baud_base) ||
      (new_serial.type != state->type) ||
      (new_serial.close_delay != state->close_delay) ||
      (new_serial.xmit_fifo_size != state->xmit_fifo_size) ||
      ((new_serial.flags &~ASYNC_USR_MASK) != (state->flags &~ASYNC_USR_MASK))
      ) return -EPERM;
    state->flags = ((state->flags &~ASYNC_USR_MASK) | (new_serial.flags & ASYNC_USR_MASK));
    info->flags = ((info->flags &~ASYNC_USR_MASK) | (new_serial.flags & ASYNC_USR_MASK));
    state->custom_divisor = new_serial.custom_divisor;
    goto check_and_exit;
  }

  new_serial.irq = irq_cannonicalize(new_serial.irq);

  if
    (
    (new_serial.irq >= NR_IRQS) ||
    (new_serial.irq < 0) ||
    (new_serial.baud_base < 9600) ||
    (new_serial.type < PORT_UNKNOWN) ||
    (new_serial.type > PORT_MAX) ||
    (new_serial.type == PORT_CIRRUS) ||
    (new_serial.type == PORT_STARTECH)
    ) {
    return -EINVAL;
  }

  if((new_serial.type != state->type) || (new_serial.xmit_fifo_size <= 0))
    new_serial.xmit_fifo_size = uart_config[new_serial.type].dfl_xmit_fifo_size;

  /* Make sure address is not already in use */
  if(new_serial.type) {
    for(i = 0; i < NR_PORTS; i++)
      if((state != &uart_table[i]) && (uart_table[i].port == new_port) && uart_table[i].type) return -EADDRINUSE;
  }

  if((change_port || change_irq) && (state->count > 1)) return -EBUSY;

  /*
   * OK, past this point, all the error checking has been done.
   * At this point, we start making changes.....
   */
  state->baud_base = new_serial.baud_base;
  state->flags = ((state->flags &~ASYNC_FLAGS) | (new_serial.flags & ASYNC_FLAGS));
  info->flags = ((state->flags &~ASYNC_INTERNAL_FLAGS) | (info->flags & ASYNC_INTERNAL_FLAGS));
  state->custom_divisor = new_serial.custom_divisor;
  state->close_delay = new_serial.close_delay * HZ / 100;
  state->closing_wait = new_serial.closing_wait * HZ / 100;
#if (LINUX_VERSION_CODE > 0x20100)
  info->tty->low_latency = (info->flags & ASYNC_LOW_LATENCY) ? 1 : 0;
#endif
  info->xmit_fifo_size = state->xmit_fifo_size = new_serial.xmit_fifo_size;

  if((state->type != PORT_UNKNOWN) && state->port) {
    release_region(state->port, 8);
  }

  state->type = new_serial.type;
  if(change_port || change_irq) {

    /*
     * We need to shutdown the serial port at the old
     * port/irq combination.
     */
    shutdown(info);
    state->irq = new_serial.irq;
    info->port = state->port = new_port;
    info->hub6 = state->hub6 = new_serial.hub6;
    if(info->hub6)
      info->io_type = state->io_type = SERIAL_IO_HUB6;
    else if(info->io_type == SERIAL_IO_HUB6)
      info->io_type = state->io_type = SERIAL_IO_PORT;
  }

  if((state->type != PORT_UNKNOWN) && state->port) {
    request_region(state->port, 8, "serial(set)");
  }

  check_and_exit:
  if(!state->port || !state->type) return 0;
  if(info->flags & ASYNC_INITIALIZED) {
    if
      (
      ((old_state.flags & ASYNC_SPD_MASK) != (state->flags & ASYNC_SPD_MASK)) ||
      (old_state.custom_divisor != state->custom_divisor)
      ) {
#if (LINUX_VERSION_CODE >= 131394)	/* Linux 2.1.66 */
      if((state->flags & ASYNC_SPD_MASK) == ASYNC_SPD_HI) info->tty->alt_speed = 57600;
      if((state->flags & ASYNC_SPD_MASK) == ASYNC_SPD_VHI) info->tty->alt_speed = 115200;
      if((state->flags & ASYNC_SPD_MASK) == ASYNC_SPD_SHI) info->tty->alt_speed = 230400;
      if((state->flags & ASYNC_SPD_MASK) == ASYNC_SPD_WARP) info->tty->alt_speed = 460800;
#endif
      change_speed(info, 0);
    }
  } else
    retval = startup(info);

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - retval=%d\n", __FUNCTION__, retval);
#endif

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
static int get_lsr_info(struct async_struct *info, unsigned int *value)
{
  unsigned char status = 0;
  unsigned int  result;
  unsigned long flags;

  save_flags_cli(flags);

  //	status = PL_UARTReadByte(PL_UART_LSR);	//serial_in(info, UART_LSR);
  restore_flags(flags);
  result = ((status & UART_LSR_TEMT) ? TIOCSER_TEMT : 0);

  /*
   * If we're about to load something into the transmit
   * register, we'll pretend the transmitter isn't empty to
   * avoid a race condition (depending on when the transmit
   * interrupt happens).
   */
  if
    (
    info->x_char ||
    (
    (CIRC_CNT(info->xmit.head, info->xmit.tail, SERIAL_XMIT_SIZE) > 0) &&
    !info->tty->stopped &&
    !info->tty->hw_stopped
    )
    ) result &= TIOCSER_TEMT;

  if(copy_to_user(value, &result, sizeof(int))) return -EFAULT;
  return 0;
}

static int get_modem_info(struct async_struct *info, unsigned int *value)
{
  unsigned char control, status = 0;
  unsigned int  result;
  unsigned long flags;
  ACIOStatus    as;
  //unsigned int regVal;

#ifdef SERIAL_DEBUG_IOCTL
  dbg_out(KERN_INFO "%s - tts/s%d\n", __FUNCTION__, info->line);
#endif

  control = info->MCR;
  save_flags_cli(flags);

  // Get CTS
  UART_GetACIOStatus((UART_REGS *)info->state->iomem_base, ACIO_CTS, &as);
  if(as.V)
    status &= ~UART_MSR_CTS;
  else
    status |= UART_MSR_CTS;

#if 0       // GPIO is disabled
  // Get DSR, DCD, RI
  regVal = readw(GPIO_DI);
  if((regVal & PIN_DSR) == PIN_DSR)
    status &= ~UART_MSR_DSR;
  else
    status |= UART_MSR_DSR;

  if((regVal & PIN_DCD) == PIN_DCD)
    status &= ~UART_MSR_DCD;
  else
    status |= UART_MSR_DCD;

  if((regVal & PIN_RI) == PIN_RI)
    status &= ~UART_MSR_RI;
  else
    status |= UART_MSR_RI;
#endif

  restore_flags(flags);
  result =    ((control & UART_MCR_RTS) ? TIOCM_RTS : 0) 
            | ((control & UART_MCR_DTR) ? TIOCM_DTR : 0)
#ifdef TIOCM_OUT1
            | ((control & UART_MCR_OUT1) ? TIOCM_OUT1 : 0) 
            | ((control & UART_MCR_OUT2) ? TIOCM_OUT2 : 0)
#endif
            | ((status & UART_MSR_DCD) ? TIOCM_CAR : 0) 
            | ((status & UART_MSR_RI) ? TIOCM_RNG : 0)
            | ((status & UART_MSR_DSR) ? TIOCM_DSR : 0) 
            | ((status & UART_MSR_CTS) ? TIOCM_CTS : 0);

  if(copy_to_user(value, &result, sizeof(int))) return -EFAULT;

#ifdef SERIAL_DEBUG_IOCTL
  dbg_out(KERN_INFO "%s successfully - result: 0x%X, GPIO reg: 0x%X\n", __FUNCTION__, result, regVal);
#endif

  return 0;
}

static int set_modem_info(struct async_struct *info, unsigned int cmd, unsigned int *value)
{
  unsigned int  arg;
  unsigned long flags;

#ifdef SERIAL_DEBUG_IOCTL
  dbg_out(KERN_INFO "%s - tts/s%d, cmd: 0x%X\n", __FUNCTION__, info->line, cmd);
#endif

  if(copy_from_user(&arg, value, sizeof(int))) return -EFAULT;

  switch(cmd) {
    case TIOCMBIS:
      if(arg & TIOCM_RTS) info->MCR |= UART_MCR_RTS;
      if(arg & TIOCM_DTR) info->MCR |= UART_MCR_DTR;
#ifdef TIOCM_OUT1
      if(arg & TIOCM_OUT1) info->MCR |= UART_MCR_OUT1;
      if(arg & TIOCM_OUT2) info->MCR |= UART_MCR_OUT2;
#endif
      if(arg & TIOCM_LOOP) info->MCR |= UART_MCR_LOOP;
      break;

    case TIOCMBIC:
      if(arg & TIOCM_RTS) info->MCR &= ~UART_MCR_RTS;
      if(arg & TIOCM_DTR) info->MCR &= ~UART_MCR_DTR;
#ifdef TIOCM_OUT1
      if(arg & TIOCM_OUT1) info->MCR &= ~UART_MCR_OUT1;
      if(arg & TIOCM_OUT2) info->MCR &= ~UART_MCR_OUT2;
#endif
      if(arg & TIOCM_LOOP) info->MCR &= ~UART_MCR_LOOP;
      break;

    case TIOCMSET:
      info->MCR = ((info->MCR &~(UART_MCR_RTS |
#ifdef TIOCM_OUT1
                                 UART_MCR_OUT1 | UART_MCR_OUT2 |
#endif
                                 UART_MCR_LOOP | UART_MCR_DTR)) | ((arg & TIOCM_RTS) ? UART_MCR_RTS : 0)
#ifdef TIOCM_OUT1
                   | ((arg & TIOCM_OUT1) ? UART_MCR_OUT1 : 0) | ((arg & TIOCM_OUT2) ? UART_MCR_OUT2 : 0)
#endif
                   | ((arg & TIOCM_LOOP) ? UART_MCR_LOOP : 0) | ((arg & TIOCM_DTR) ? UART_MCR_DTR : 0));
      break;

    default:
      return -EINVAL;
  }

  save_flags_cli(flags);
  info->MCR |= ALPHA_KLUDGE_MCR;  /* Don't ask */

  ProcessDTR(info);
  ProcessRTS(info);

  restore_flags(flags);
  return 0;
}

static int do_autoconfig(struct async_struct *info)
{
  int irq, retval;

  if(!capable(CAP_SYS_ADMIN)) return -EPERM;
  if(info->state->count > 1) return -EBUSY;

  shutdown(info);

  autoconfig(info->state);
  if((info->state->flags & ASYNC_AUTO_IRQ) && (info->state->port != 0) && (info->state->type != PORT_UNKNOWN)) {
    irq = detect_uart_irq(info->state);
    if(irq > 0) info->state->irq = irq;
  }

  retval = startup(info);
  if(retval) return retval;
  return 0;
}

/*
 * uart_break() --- routine which turns the break handling on or off
 */
static void uart_break(struct tty_struct *tty, int break_state)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  unsigned long       flags;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d, state=%d\n", __FUNCTION__, info->line, break_state);
#endif

  if(serial_paranoia_check(info, tty->device, "uart_break")) return;
  if(!CONFIGURED_SERIAL_PORT(info)) return;
  save_flags_cli(flags);
  if(break_state == -1)
    info->LCR |= UART_LCR_SBC;
  else
    info->LCR &= ~UART_LCR_SBC;

  //	PL_UARTWriteByte(info->LCR, PL_UART_LCR);
  //	serial_out(info, UART_LCR, info->LCR);
  restore_flags(flags);

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

static int uart_ioctl(struct tty_struct *tty, struct file *file, unsigned int cmd, unsigned long arg)
{

  struct async_struct           *info = (struct async_struct *)tty->driver_data;
  struct async_icount           cprev, cnow;  /* kernel counter temps */
  struct serial_icounter_struct icount;
  unsigned long                 flags;
  volatile UART_REGS                *UartBaseAddr=(UART_REGS *)info->state->iomem_base;
#if (LINUX_VERSION_CODE < 131394)							/* Linux 2.1.66 */
  int                           retval, tmp;
#endif

  if(serial_paranoia_check(info, tty->device, "uart_ioctl")) return -ENODEV;

  if
    (
    (cmd != TIOCGSERIAL) &&
    (cmd != TIOCSSERIAL) &&
    (cmd != TIOCSERCONFIG) &&
    (cmd != TIOCSERGSTRUCT) &&
    (cmd != TIOCMIWAIT) &&
    (cmd != TIOCGICOUNT)
    ) {
    if(tty->flags & (1 << TTY_IO_ERROR)) return -EIO;
  }

#ifdef SERIAL_DEBUG_IOCTL
  dbg_out(KERN_INFO "%s - cmd=0x%04X\n", __FUNCTION__, cmd);
#endif

  switch(cmd) {
#if (LINUX_VERSION_CODE < 131394)						/* Linux 2.1.66 */

    case TCSBRK:                    /* SVID version: non-zero arg --> no break */
      retval = tty_check_change(tty);
      if(retval) return retval;
      tty_wait_until_sent(tty, 0);
      if(signal_pending(current)) return -EINTR;
      if(!arg) {
        send_break(info, HZ / 4);   /* 1/4 second */
        if(signal_pending(current)) return -EINTR;
      }

      return 0;

    case TCSBRKP:                   /* support for POSIX tcsendbreak() */
      retval = tty_check_change(tty);
      if(retval) return retval;
      tty_wait_until_sent(tty, 0);
      if(signal_pending(current)) return -EINTR;
      send_break(info, arg ? arg * (HZ / 10) : HZ / 4);
      if(signal_pending(current)) return -EINTR;
      return 0;

    case TIOCGSOFTCAR:
      tmp = C_CLOCAL(tty) ? 1 : 0;
      if(copy_to_user((void *)arg, &tmp, sizeof(int))) return -EFAULT;
      return 0;

    case TIOCSSOFTCAR:
      if(copy_from_user(&tmp, (void *)arg, sizeof(int))) return -EFAULT;

      tty->termios->c_cflag = ((tty->termios->c_cflag &~CLOCAL) | (tmp ? CLOCAL : 0));
      return 0;
#endif
    case TIOCMGET:
      return get_modem_info(info, (unsigned int *)arg);

    case TIOCMBIS:
#if RS485
      UART_SetACIOControl(UartBaseAddr, ACIO_RTS, 3, 2);  // set RTS
      return 0;
#endif

    case TIOCMBIC:
#if RS485
      UART_SetACIOControl(UartBaseAddr, ACIO_RTS, 3, 0);  // clr RTS
      return 0;
#endif

    case TIOCMSET:
      return set_modem_info(info, cmd, (unsigned int *)arg);

    case TIOCGSERIAL:
      return get_serial_info(info, (struct serial_struct *)arg);

    case TIOCSSERIAL:
      return set_serial_info(info, (struct serial_struct *)arg);

    case TIOCSERCONFIG:
      return do_autoconfig(info);

    case TIOCSERGETLSR:             /* Get line status register */
      return get_lsr_info(info, (unsigned int *)arg);

    case TIOCSERGSTRUCT:
      if(copy_to_user((struct async_struct *)arg, info, sizeof(struct async_struct))) return -EFAULT;
      return 0;

      /*
       * Wait for any of the 4 modem inputs (DCD,RI,DSR,CTS) to change
       * - mask passed in arg for lines of interest
       *   (use |'ed TIOCM_RNG/DSR/CD/CTS for masking)
       * Caller should use TIOCGICOUNT to see which one it was
       */
    case TIOCMIWAIT:
      save_flags(flags);
      cli();

      /* note the counters on entry */
      cprev = info->state->icount;
      restore_flags(flags);

      /* Force modem status interrupts on */
      info->IER |= UART_IER_MSI;

      //			PL_UARTWriteByte(info->IER, PL_UART_IER);
      //serial_out(info, UART_IER, info->IER);
      while(1) {
        interruptible_sleep_on(&info->delta_msr_wait);

        /* see if a signal did it */
        if(signal_pending(current)) return -ERESTARTSYS;
        save_flags(flags);
        cli();
        cnow = info->state->icount; /* atomic copy */
        restore_flags(flags);
        if(cnow.rng == cprev.rng && cnow.dsr == cprev.dsr && cnow.dcd == cprev.dcd && cnow.cts == cprev.cts)
          return -EIO;              /* no change => error */
        if
          (
          ((arg & TIOCM_RNG) && (cnow.rng != cprev.rng)) ||
          ((arg & TIOCM_DSR) && (cnow.dsr != cprev.dsr)) ||
          ((arg & TIOCM_CD) && (cnow.dcd != cprev.dcd)) ||
          ((arg & TIOCM_CTS) && (cnow.cts != cprev.cts))
          ) {
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
      save_flags(flags);
      cli();
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

      if(copy_to_user((void *)arg, &icount, sizeof(icount))) return -EFAULT;
      return 0;

    case TIOCSERGWILD:
    case TIOCSERSWILD:
      /* "setserial -W" is called in Debian boot */
      dbg_out(KERN_INFO "TIOCSER?WILD ioctl obsolete, ignored.\n");
      return 0;

    default:
#ifdef SERIAL_DEBUG_IOCTL
      dbg_out(KERN_INFO "%s - cmd=0x%04X is not handled\n", __FUNCTION__, cmd);
#endif
      return -ENOIOCTLCMD;
  }

  return 0;
}

static void uart_set_termios(struct tty_struct *tty, struct termios *old_termios)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  unsigned long       flags;
  unsigned int        cflag = tty->termios->c_cflag;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d, count = %d\n", __FUNCTION__, info->line, info->state->count);
#endif

  if((cflag == old_termios->c_cflag) && (RELEVANT_IFLAG(tty->termios->c_iflag) == RELEVANT_IFLAG(old_termios->c_iflag)))
    return;

  change_speed(info, old_termios);

  /* Handle transition to B0 status */
  if((old_termios->c_cflag & CBAUD) && !(cflag & CBAUD)) {
    info->MCR &= ~(UART_MCR_DTR | UART_MCR_RTS);
    save_flags_cli(flags);
    ProcessDTR(info);
    ProcessRTS(info);
    restore_flags(flags);
  }

  /* Handle transition away from B0 status */
  if(!(old_termios->c_cflag & CBAUD) && (cflag & CBAUD)) {
    info->MCR |= UART_MCR_DTR;
    if(!(tty->termios->c_cflag & CRTSCTS) || !test_bit(TTY_THROTTLED, &tty->flags)) {
      info->MCR |= UART_MCR_RTS;
    }

    save_flags_cli(flags);
    ProcessDTR(info);
    ProcessRTS(info);
    restore_flags(flags);
  }

  /* Handle turning off CRTSCTS */
  if((old_termios->c_cflag & CRTSCTS) && !(tty->termios->c_cflag & CRTSCTS)) {
    tty->hw_stopped = 0;
    uart_start(tty);
  }

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

/*
 * ------------------------------------------------------------
 * uart_close()
 *
 * This routine is called when the serial port gets closed.  First, we
 * wait for the last remaining data to be sent.  Then, we unlink its
 * async structure from the interrupt chain if necessary, and we free
 * that IRQ if nothing is left in the chain.
 * ------------------------------------------------------------
 */
static void uart_close(struct tty_struct *tty, struct file *filp)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  struct serial_state *state;
  unsigned long       flags;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d\n", __FUNCTION__, info->line);
#endif

  if(!info || serial_paranoia_check(info, tty->device, "uart_close")) return;

  state = info->state;
  save_flags_cli(flags);
  if(tty_hung_up_p(filp)) {
    MOD_DEC_USE_COUNT;
    restore_flags(flags);
    return;
  }

  if((tty->count == 1) && (state->count != 1)) {

    /*
     * Uh, oh.  tty->count is 1, which means that the tty
     * structure will be freed.  state->count should always
     * be one in these conditions.  If it's greater than
     * one, we've got real problems, since it means the
     * serial port won't be shutdown.
     */
    dbg_out(KERN_INFO "%s - bad serial port count; tty->count is 1, state->count is %d\n", __FUNCTION__, state->count);
    state->count = 1;
  }

  if(--state->count < 0) {
    dbg_out(KERN_INFO "%s - bad serial port count for tts/s%d: %d\n", __FUNCTION__, info->line, state->count);
    state->count = 0;
  }

  if(state->count) {
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
  if(info->flags & ASYNC_NORMAL_ACTIVE) info->state->normal_termios = *tty->termios;
  if(info->flags & ASYNC_CALLOUT_ACTIVE) info->state->callout_termios = *tty->termios;

  /*
   * Now we wait for the transmit buffer to clear; and we notify
   * the line discipline to only process XON/XOFF characters.
   */
  tty->closing = 1;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - before tty_wait_until_sent\n", __FUNCTION__);
#endif

  if(info->closing_wait != ASYNC_CLOSING_WAIT_NONE) tty_wait_until_sent(tty, info->closing_wait);

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - after tty_wait_until_sent\n", __FUNCTION__);
#endif

  /*
   * At this point we stop accepting input.  To do this, we
   * disable the receive line status interrupts, and tell the
   * interrupt driver to stop checking the data ready bit in the
   * line status register.
   */
  info->IER &= ~UART_IER_RLSI;
  info->read_status_mask &= ~UART_LSR_DR;
  if(info->flags & ASYNC_INITIALIZED) {

    //		PL_UARTWriteByte(info->IER, PL_UART_IER);
    //serial_out(info, UART_IER, info->IER);
    /*
     * Before we drop DTR, make sure the UART transmitter
     * has completely drained; this is especially
     * important if there is a transmit FIFO!
     */
    uart_wait_until_sent(tty, info->timeout);
  }

  shutdown(info);

  if(tty->driver.flush_buffer) tty->driver.flush_buffer(tty);
  if(tty->ldisc.flush_buffer) tty->ldisc.flush_buffer(tty);
  tty->closing = 0;
  info->event = 0;
  info->tty = 0;
  if(info->blocked_open) {
    if(info->close_delay) {
      set_current_state(TASK_INTERRUPTIBLE);
      schedule_timeout(info->close_delay);
    }

    wake_up_interruptible(&info->open_wait);
  }

#if DMA_OUT_OPEN
  if(gDMAWriteBuffer) free_page((unsigned long)gDMAWriteBuffer);
#endif
#if DMA_IN_OPEN
  if(gDMAReadBuffer) free_page((unsigned long)gDMAReadBuffer);
#endif
  if(gTmpBuffer) kfree(gTmpBuffer);

  info->flags &= ~(ASYNC_NORMAL_ACTIVE | ASYNC_CALLOUT_ACTIVE | ASYNC_CLOSING);
  wake_up_interruptible(&info->close_wait);
  MOD_DEC_USE_COUNT;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

/*
 * uart_wait_until_sent() --- wait until the transmitter is empty
 */
static void uart_wait_until_sent(struct tty_struct *tty, int timeout)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  unsigned long       orig_jiffies, char_time;

  //	int									lsr;
#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - timeout=%d\n", __FUNCTION__, timeout);
#endif

  if(serial_paranoia_check(info, tty->device, "uart_wait_until_sent")) return;
  if(info->state->type == PORT_UNKNOWN) return;
  if(info->xmit_fifo_size == 0) return; /* Just in case.... */

  orig_jiffies = jiffies;

  /*
   * Set the check interval to be 1/5 of the estimated time to
   * send a single character, and make it at least 1.  The check
   * interval should also be less than the timeout.
   *
   * Note: we have to use pretty tight timings here to satisfy
   * the NIST-PCTS.
   */
  char_time = (info->timeout - HZ / 50) / info->xmit_fifo_size;
  char_time = char_time / 5;
  if(char_time == 0) char_time = 1;
  if(timeout && timeout < char_time) char_time = timeout;

  /*
   * If the transmitter hasn't cleared in twice the approximate
   * amount of time to send the entire FIFO, it probably won't
   * ever clear.  This assumes the UART isn't doing flow
   * control, which is currently the case.  Hence, if it ever
   * takes longer than info->timeout, this is probably due to a
   * UART bug of some kind.  So, we clamp the timeout parameter at
   * 2*info->timeout.
   */
  if(!timeout || timeout > 2 * info->timeout) timeout = 2 * info->timeout;
#ifdef SERIAL_DEBUG_RS_WAIT_UNTIL_SENT
  dbg_out(KERN_INFO "%s - timeout=%d char_time=%lu\n", __FUNCTION__, timeout, char_time);
  dbg_out(KERN_INFO "jiff=%lu...\n", jiffies);
#endif
  /*
  while(!((lsr = PL_UARTReadByte(PL_UART_LSR)) & UART_LSR_TEMT))	{
#ifdef SERIAL_DEBUG_RS_WAIT_UNTIL_SENT
    dbg_out(KERN_INFO "lsr=%d (jiff=%lu)...\n", lsr, jiffies);
#endif
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(char_time);
    if(signal_pending(current)) break;
    if(timeout && time_after(jiffies, orig_jiffies + timeout)) break;
  }
*/
#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

/*
 * uart_hangup() --- called by tty_hangup() when a hangup is signaled.
 */
static void uart_hangup(struct tty_struct *tty)
{
  struct async_struct *info = (struct async_struct *)tty->driver_data;
  struct serial_state *state = info->state;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d, count = %d\n", __FUNCTION__, info->line, info->state->count);
#endif

  if(serial_paranoia_check(info, tty->device, "uart_hangup")) return;

  state = info->state;

  uart_flush_buffer(tty);
  if(info->flags & ASYNC_CLOSING) return;
  shutdown(info);
  info->event = 0;
  state->count = 0;
  info->flags &= ~(ASYNC_NORMAL_ACTIVE | ASYNC_CALLOUT_ACTIVE);
  info->tty = 0;
  wake_up_interruptible(&info->open_wait);
#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif
}

/*
 * ------------------------------------------------------------
 * uart_open() and friends
 * ------------------------------------------------------------
 */
static int block_til_ready(struct tty_struct *tty, struct file *filp, struct async_struct *info)
{
  DECLARE_WAITQUEUE(wait, current);

  struct serial_state *state = info->state;
  int                 retval;
  int                 do_clocal = 0, extra_count = 0;
  unsigned long       flags;

  /*
   * If the device is in the middle of being closed, then block
   * until it's done, and then try again.
   */
  if(tty_hung_up_p(filp) || (info->flags & ASYNC_CLOSING)) {
    if(info->flags & ASYNC_CLOSING) interruptible_sleep_on(&info->close_wait);
#ifdef SERIAL_DO_RESTART
    return((info->flags & ASYNC_HUP_NOTIFY) ? -EAGAIN : -ERESTARTSYS);
#else
    return -EAGAIN;
#endif
  }

  /*
   * If this is a callout device, then just make sure the normal
   * device isn't being used.
   */
  if(tty->driver.subtype == SERIAL_TYPE_CALLOUT) {
    if(info->flags & ASYNC_NORMAL_ACTIVE) return -EBUSY;
    if
      (
      (info->flags & ASYNC_CALLOUT_ACTIVE) &&
      (info->flags & ASYNC_SESSION_LOCKOUT) &&
      (info->session != current->session)
      ) return -EBUSY;
    if((info->flags & ASYNC_CALLOUT_ACTIVE) && (info->flags & ASYNC_PGRP_LOCKOUT) && (info->pgrp != current->pgrp))
      return -EBUSY;
    info->flags |= ASYNC_CALLOUT_ACTIVE;
    return 0;
  }

  /*
   * If non-blocking mode is set, or the port is not enabled,
   * then make the check up front and then exit.
   */
  if((filp->f_flags & O_NONBLOCK) || (tty->flags & (1 << TTY_IO_ERROR))) {
    if(info->flags & ASYNC_CALLOUT_ACTIVE) return -EBUSY;
    info->flags |= ASYNC_NORMAL_ACTIVE;
    return 0;
  }

  if(info->flags & ASYNC_CALLOUT_ACTIVE) {
    if(state->normal_termios.c_cflag & CLOCAL) do_clocal = 1;
  } else {
    if(tty->termios->c_cflag & CLOCAL) do_clocal = 1;
  }

  /*
   * Block waiting for the carrier detect and the line to become
   * free (i.e., not in use by the callout).  While we are in
   * this loop, state->count is dropped by one, so that
   * uart_close() knows when to free things.  We restore it upon
   * exit, either normal or abnormal.
   */
  retval = 0;
  add_wait_queue(&info->open_wait, &wait);
#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - before block: tts/s%d, count = %d\n", __FUNCTION__, state->line, state->count);
#endif
  save_flags_cli(flags);
  if(!tty_hung_up_p(filp)) {
    extra_count = 1;
    state->count--;
  }

  restore_flags(flags);
  info->blocked_open++;
  while(1) {
    save_flags_cli(flags);
    if(!(info->flags & ASYNC_CALLOUT_ACTIVE) && (tty->termios->c_cflag & CBAUD)) {

      //			PL_UARTWriteByte(PL_UARTReadByte(PL_UART_MCR) | (UART_MCR_DTR | UART_MCR_RTS), PL_UART_MCR);
      //			serial_out(info, UART_MCR, serial_inp(info, UART_MCR) | (UART_MCR_DTR | UART_MCR_RTS));
    }

    restore_flags(flags);
    set_current_state(TASK_INTERRUPTIBLE);
    if(tty_hung_up_p(filp) || !(info->flags & ASYNC_INITIALIZED)) {
#ifdef SERIAL_DO_RESTART
      if(info->flags & ASYNC_HUP_NOTIFY)
        retval = -EAGAIN;
      else
        retval = -ERESTARTSYS;
#else
      retval = -EAGAIN;
#endif
      break;
    }

    if(!(info->flags & ASYNC_CALLOUT_ACTIVE) && !(info->flags & ASYNC_CLOSING) && do_clocal) break;
    if(signal_pending(current)) {
      retval = -ERESTARTSYS;
      break;
    }

#ifdef SERIAL_DEBUG_OPEN
    dbg_out(KERN_INFO "%s - blocking: tts/s%d, count = %d\n", __FUNCTION__, info->line, state->count);
#endif
    schedule();
  }

  set_current_state(TASK_RUNNING);
  remove_wait_queue(&info->open_wait, &wait);
  if(extra_count) state->count++;
  info->blocked_open--;
#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - after block: tts/s%d, count = %d\n", __FUNCTION__, info->line, state->count);
#endif
  if(retval) return retval;
  info->flags |= ASYNC_NORMAL_ACTIVE;
  return 0;
}

static int get_async_struct(int line, struct async_struct **ret_info)
{
  struct async_struct *info;
  struct serial_state *sstate;

  sstate = uart_table + line;
  sstate->count++;
  if(sstate->info) {
    *ret_info = sstate->info;
    return 0;
  }

  info = kmalloc(sizeof(struct async_struct), GFP_KERNEL);
  if(!info) {
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
  if(sstate->info) {
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
 */
static int uart_open(struct tty_struct *tty, struct file *filp)
{
  struct async_struct *info;
  int                 retval, line;
  unsigned long       page;

  MOD_INC_USE_COUNT;
  line = MINOR(tty->device) - tty->driver.minor_start;
  if((line < 0) || (line >= NR_PORTS)) {
    MOD_DEC_USE_COUNT;
    return -ENODEV;
  }

  retval = get_async_struct(line, &info);
  if(retval) {
    MOD_DEC_USE_COUNT;
    return retval;
  }

  tty->driver_data = info;
  info->tty = tty;
  if(serial_paranoia_check(info, tty->device, "uart_open")) {
    MOD_DEC_USE_COUNT;
    return -ENODEV;
  }

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - tts/s%d, count = %d\n", __FUNCTION__, info->line, info->state->count);
#endif
#if (LINUX_VERSION_CODE > 0x20100)
  info->tty->low_latency = (info->flags & ASYNC_LOW_LATENCY) ? 1 : 0;
#endif

  if(!tmp_buf) {
    page = get_zeroed_page(GFP_KERNEL);
    if(!page) {
      MOD_DEC_USE_COUNT;
      return -ENOMEM;
    }

    if(tmp_buf)
      free_page(page);
    else
      tmp_buf = (unsigned char *)page;
  }

  /*
   * If the port is the middle of closing, bail out now
   */
  if(tty_hung_up_p(filp) || (info->flags & ASYNC_CLOSING)) {
    if(info->flags & ASYNC_CLOSING) interruptible_sleep_on(&info->close_wait);
    MOD_DEC_USE_COUNT;
#ifdef SERIAL_DO_RESTART
    return((info->flags & ASYNC_HUP_NOTIFY) ? -EAGAIN : -ERESTARTSYS);
#else
    return -EAGAIN;
#endif
  }

  /*
   * Start up serial port
   */
  retval = startup(info);
  if(retval) {
    MOD_DEC_USE_COUNT;
    return retval;
  }

  retval = block_til_ready(tty, filp, info);
  if(retval) {
    MOD_DEC_USE_COUNT;

#ifdef SERIAL_DEBUG_OPEN
    dbg_out(KERN_INFO "%s - returning after block_til_ready with %d\n", __FUNCTION__, retval);
#endif

    return retval;
  }

  if((info->state->count == 1) && (info->flags & ASYNC_SPLIT_TERMIOS)) {
    if(tty->driver.subtype == SERIAL_TYPE_NORMAL)
      *tty->termios = info->state->normal_termios;
    else
      *tty->termios = info->state->callout_termios;
    change_speed(info, 0);
  }

  info->session = current->session;
  info->pgrp = current->pgrp;

#ifdef SERIAL_DEBUG_OPEN
  dbg_out(KERN_INFO "%s - exit successfully\n", __FUNCTION__);
#endif

  return 0;
}

/*
 * /proc fs routines....
 */
static inline int line_info(char *buf, struct serial_state *state)
{
  struct async_struct *info = state->info, scr_info;
  char                stat_buf[30], control = 0, status = 0;
  int                 ret;
  unsigned long       flags;
  ACIOStatus          as;

  ret = sprintf(buf, "%d: uart:%s port:%lX irq:%d", state->line, uart_config[state->type].name, state->port, state->irq);

  if(!state->port || (state->type == PORT_UNKNOWN)) {
    ret += sprintf(buf + ret, "\n");
    return ret;
  }

  /*
   * Figure out the current RS-232 lines
   */
  if(!info) {
    info = &scr_info;           /* This is just for serial_{in,out} */

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

  save_flags_cli(flags);

  if(info->MCR & UART_MCR_RTS) control |= UART_MCR_RTS;

  UART_GetACIOStatus((UART_REGS *)info->state->iomem_base, ACIO_CTS, &as);
  if(as.V)
    status &= ~UART_MSR_CTS;
  else
    status |= UART_MSR_CTS;

  restore_flags(flags);

  stat_buf[0] = 0;
  stat_buf[1] = 0;
  if(control & UART_MCR_RTS) strcat(stat_buf, "|RTS");
  if(status & UART_MSR_CTS) strcat(stat_buf, "|CTS");
  if(control & UART_MCR_DTR) strcat(stat_buf, "|DTR");
  if(status & UART_MSR_DSR) strcat(stat_buf, "|DSR");
  if(status & UART_MSR_DCD) strcat(stat_buf, "|CD");
  if(status & UART_MSR_RI) strcat(stat_buf, "|RI");

  if(info->quot) {
    ret += sprintf(buf + ret, " baud:%d", state->baud_base / info->quot);
  }

  ret += sprintf(buf + ret, " tx:%d rx:%d", state->icount.tx, state->icount.rx);

  if(state->icount.frame) ret += sprintf(buf + ret, " fe:%d", state->icount.frame);
  if(state->icount.parity) ret += sprintf(buf + ret, " pe:%d", state->icount.parity);
  if(state->icount.brk) ret += sprintf(buf + ret, " brk:%d", state->icount.brk);
  if(state->icount.overrun) ret += sprintf(buf + ret, " oe:%d", state->icount.overrun);

  /*
   * Last thing is the RS-232 status lines
   */
  ret += sprintf(buf + ret, " %s\n", stat_buf + 1);
  return ret;
}

int uart_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
  int   i, len = 0, l;
  off_t begin = 0;

#ifdef SERIAL_DEBUG_READ
  dbg_out(KERN_INFO "%s - count=%d\n", __FUNCTION__, count);
#endif

  len += sprintf(page, "serinfo:1.0 driver:%s%s revision:%s\n", serial_version, LOCAL_VERSTRING, serial_revdate);
  for(i = 0; i < NR_PORTS && len < 4000; i++) {
    l = line_info(page + len, &uart_table[i]);
    len += l;
    if(len + begin > off + count) goto done;
    if(len + begin < off) {
      begin += len;
      len = 0;
    }
  }

  *eof = 1;
  done:
  if(off >= len + begin) return 0;
  *start = page + (off - begin);
  return((count < begin + len - off) ? count : begin + len - off);
}

static inline void show_serial_version(void)
{
  printk(KERN_INFO "%s version %s%s (%s)\n", serial_name, serial_version, LOCAL_VERSTRING, serial_revdate);
}

static unsigned detect_uart_irq(struct serial_state *state)
{
  int                 irq;
  unsigned long       irqs;

  //	unsigned char				save_mcr, save_ier;
  struct async_struct scr_info; /* serial_{in,out} because HUB6 */

  scr_info.magic = SERIAL_MAGIC;
  scr_info.state = state;
  scr_info.port = state->port;
  scr_info.flags = state->flags;
#ifdef CONFIG_HUB6
  scr_info.hub6 = state->hub6;
#endif
  scr_info.io_type = state->io_type;
  scr_info.iomem_base = state->iomem_base;
  scr_info.iomem_reg_shift = state->iomem_reg_shift;

  /* forget possible initially masked and pending IRQ */
  probe_irq_off(probe_irq_on());

  /*
  save_mcr = PL_UARTReadByte(PL_UART_MCR);
  save_ier = PL_UARTReadByte(PL_UART_IER);
  PL_UARTWriteByte(UART_MCR_OUT1 | UART_MCR_OUT2, PL_UART_MCR);
*/
  //	save_mcr = serial_inp(&scr_info, UART_MCR);
  //	save_ier = serial_inp(&scr_info, UART_IER);
  //	serial_outp(&scr_info, UART_MCR, UART_MCR_OUT1 | UART_MCR_OUT2);
  irqs = probe_irq_on();

  //	PL_UARTWriteByte(0x00, PL_UART_MCR);
  //	serial_outp(&scr_info, UART_MCR, 0);
  udelay(10);

  /*
  if(state->flags & ASYNC_FOURPORT) {
    PL_UARTWriteByte(UART_MCR_DTR | UART_MCR_RTS, PL_UART_MCR);

    //		serial_outp(&scr_info, UART_MCR,
    //			    UART_MCR_DTR | UART_MCR_RTS);
  } else {
    PL_UARTWriteByte(UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2, PL_UART_MCR);

    //		serial_outp(&scr_info, UART_MCR,
    //			    UART_MCR_DTR | UART_MCR_RTS | UART_MCR_OUT2);
  }

  PL_UARTWriteByte(0x0F, PL_UART_IER);
  (void)PL_UARTReadByte(PL_UART_LSR);
  (void)PL_UARTReadByte(PL_UART_RX);
  (void)PL_UARTReadByte(PL_UART_IIR);
  (void)PL_UARTReadByte(PL_UART_MSR);
  PL_UARTWriteByte(0xFF, PL_UART_TX);

  udelay(20);
*/
  irq = probe_irq_off(irqs);

  /*
  PL_UARTWriteByte(save_mcr, PL_UART_MCR);
  PL_UARTWriteByte(save_ier, PL_UART_IER);
*/
  //	serial_outp(&scr_info, UART_MCR, save_mcr);
  //	serial_outp(&scr_info, UART_IER, save_ier);
  return(irq > 0) ? irq : 0;
}

/*
 * This routine is called by init_uart() to initialize a specific serial
 * port.  It determines what type of UART chip this serial port is
 * using: 8250, 16450, 16550, 16550A.  The important question is
 * whether or not this UART is a 16550A or not, since this will
 * determine whether or not we can use its FIFO features or not.
 */
static void autoconfig(struct serial_state *state)
{
  state->type = PORT_UNKNOWN;

  if(!CONFIGURED_SERIAL_PORT(state)) return;

  if(state->irq == IRQ_PL_CONSOLE)
    state->type = PORT_PL_SIO;
  else if(state->irq == IRQ_PL_UART)
    state->type = PORT_PL_UART;
  else if(state->irq == IRQ_PL_UART_1)
    state->type = PORT_PL_UART_1;
}

#define SERIAL_DEV_OFFSET 1

int __init init_uart(void)
{
//printk("UART Driver Init, PL_UART_BASE is 0x%x======\n", PL_UART_BASE);
    /* for pl1029*/
#ifdef CONFIG_ARCH_PL1029
    int uart_clk=0;

    if (pl_get_dev_hz() == 96000000) {   /* dclk = 96MHz */
        writeb(4, PL_CLK_UART);          /* program uart dev clk to 48MHz */
        uart_clk = 96000000/2;
    }else if (pl_get_dev_hz() == 32000000) {    /* dclk = 32MHz */
        writeb(5, PL_CLK_UART);          /* target 24MHz */
        uart_clk = 32000000/2;
    } else if (pl_get_dev_hz() == 120000000) {  /* dclk = 120MHz */
        writeb(4, PL_CLK_UART);          /* target 48MHz */
        uart_clk = 120000000/2;
    } else {
        printk("The device clock %d is unsupported by UART driver\n", pl_get_dev_hz());
        return -EINVAL;
    }

#if     ENABLE_UART_1
    writeb(0x40, GPIO_IO_SEL);

    if (pl_get_dev_hz() == 96000000) {   /* dclk = 96MHz */
        writeb(4, PL_CLK_UART_1);          /* program uart dev clk to 48MHz */
        uart_clk = 96000000/2;
    }else if (pl_get_dev_hz() == 32000000) {    /* dclk = 32MHz */
        writeb(5, PL_CLK_UART_1);          /* target 24MHz */
        uart_clk = 32000000/2;
    } else if (pl_get_dev_hz() == 120000000) {  /* dclk = 120MHz */
        writeb(4, PL_CLK_UART_1);          /* target 48MHz */
        uart_clk = 120000000/2;
    } else {
        printk("The device clock %d is unsupported by UART driver\n", pl_get_dev_hz());
        return -EINVAL;
    }
#endif  // ENABLE_UART_1
#endif

  struct serial_state *state;
  int i;

//  init_bh(SERIAL_BH, do_serial_bh);

  /* init rs timer */
  init_timer(&serial_timer);
  serial_timer.function = uart_timer;
  mod_timer(&serial_timer, jiffies + RS_STROBE_TIME);

  for(i = 0; i < NR_IRQS; i++) {
    IRQ_ports[i] = 0;
    IRQ_timeout[i] = 0;
  }

  show_serial_version();

  memset(&serial_driver, 0, sizeof(struct tty_driver));
  serial_driver.magic = TTY_DRIVER_MAGIC;

#if (LINUX_VERSION_CODE > 0x20100)
  serial_driver.driver_name = "uart";
#endif
#if (LINUX_VERSION_CODE > 0x2032D && defined(CONFIG_DEVFS_FS))
  serial_driver.name = "tts/s%d";
#else
  serial_driver.name = "ttyS";
#endif

  serial_driver.major = TTY_MAJOR;
  serial_driver.minor_start = 64 + SERIAL_DEV_OFFSET;
  serial_driver.num = NR_PORTS;
  serial_driver.type = TTY_DRIVER_TYPE_SERIAL;
  serial_driver.subtype = SERIAL_TYPE_NORMAL;
  serial_driver.init_termios = tty_std_termios;
  serial_driver.init_termios.c_cflag = B9600 | CS8 | CREAD | HUPCL | CLOCAL;
  serial_driver.flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_NO_DEVFS;
  serial_driver.refcount = &serial_refcount;
  serial_driver.table = serial_table;
  serial_driver.termios = serial_termios;
  serial_driver.termios_locked = serial_termios_locked;

  serial_driver.open = uart_open;
  serial_driver.close = uart_close;
  serial_driver.write = uart_write;
  serial_driver.put_char = uart_put_char;
  serial_driver.flush_chars = uart_flush_chars;
  serial_driver.write_room = uart_write_room;
  serial_driver.chars_in_buffer = uart_chars_in_buffer;
  serial_driver.flush_buffer = uart_flush_buffer;
  serial_driver.ioctl = uart_ioctl;
  serial_driver.throttle = uart_throttle;
  serial_driver.unthrottle = uart_unthrottle;
  serial_driver.set_termios = uart_set_termios;
  serial_driver.stop = uart_stop;
  serial_driver.start = uart_start;
  serial_driver.hangup = uart_hangup;
#if (LINUX_VERSION_CODE >= 131394)	/* Linux 2.1.66 */
  serial_driver.break_ctl = uart_break;
#endif
#if (LINUX_VERSION_CODE >= 131343)
  serial_driver.send_xchar = uart_send_xchar;
  serial_driver.wait_until_sent = uart_wait_until_sent;
  serial_driver.read_proc = uart_read_proc;
#endif

  /*
   * The callout device is just like normal device except for
   * major number and the subtype code.
   */
  callout_driver = serial_driver;
#if (LINUX_VERSION_CODE > 0x2032D && defined(CONFIG_DEVFS_FS))
  callout_driver.name = "cua/s%d";
#else
  callout_driver.name = "cua";
#endif
  callout_driver.major = TTYAUX_MAJOR;
  callout_driver.subtype = SERIAL_TYPE_CALLOUT;
#if (LINUX_VERSION_CODE >= 131343)
  callout_driver.read_proc = 0;
  callout_driver.proc_entry = 0;
#endif

  if(tty_register_driver(&serial_driver)) {
    dbg_out(KERN_ERR "Couldn't register serial driver\n");
    return -EFAULT;
  }

  if(tty_register_driver(&callout_driver)) {
    dbg_out(KERN_ERR "Couldn't register callout driver\n");
    return -EFAULT;
  }

  for(i = 0, state = uart_table; i < NR_PORTS; i++, state++) {
    state->magic = SSTATE_MAGIC;
    state->line = i;
    state->custom_divisor = 0;
    state->close_delay = 5 * HZ / 10;
    state->closing_wait = 30 * HZ;
    state->callout_termios = callout_driver.init_termios;
    state->normal_termios = serial_driver.init_termios;
    state->icount.cts = state->icount.dsr = state->icount.rng = state->icount.dcd = 0;
    state->icount.rx = state->icount.tx = 0;
    state->icount.frame = state->icount.parity = 0;
    state->icount.overrun = state->icount.brk = 0;
    state->irq = irq_cannonicalize(state->irq);
    if(state->hub6) state->io_type = SERIAL_IO_HUB6;
    if(state->port && check_region(state->port, 8)) continue;
    if(state->flags & ASYNC_BOOT_AUTOCONF) autoconfig(state);
  }

  for(i = 0, state = uart_table; i < NR_PORTS; i++, state++) {
    if(state->type == PORT_UNKNOWN) continue;

    if(state->io_type == SERIAL_IO_MEM) {
      dbg_out(KERN_INFO "tts/s%d%s at 0x%p (irq = %d) is a %s\n", state->line + SERIAL_DEV_OFFSET - 1,
              (state->flags & ASYNC_FOURPORT) ? " FourPort" : "", (void *)state->iomem_base, state->irq,
              uart_config[state->type].name);
    } else {
      dbg_out(KERN_INFO "tts/s%d%s at 0x%04lX (irq = %d) is a %s\n", state->line + SERIAL_DEV_OFFSET - 1,
              (state->flags & ASYNC_FOURPORT) ? " FourPort" : "", state->port, state->irq, uart_config[state->type].name);
    }

    tty_register_devfs(&serial_driver, 0, serial_driver.minor_start + state->line);
    tty_register_devfs(&callout_driver, 0, callout_driver.minor_start + state->line);
  }

  return 0;
}

void __exit cleanup_uart(void)
{
  unsigned long flags;
  int           e1, e2;

  del_timer_sync(&serial_timer);
  save_flags_cli(flags);
  remove_bh(SERIAL_BH);
  if((e1 = tty_unregister_driver(&serial_driver))) {
    dbg_out(KERN_ERR "%s - failed to unregister serial driver e1=%d\n", __FUNCTION__, e1);
  }

  if((e2 = tty_unregister_driver(&callout_driver))) {
    dbg_out(KERN_ERR "%s - failed to unregister callout driver e2=%d\n", __FUNCTION__, e2);
  }

  restore_flags(flags);
}

module_init(init_uart);
module_exit(cleanup_uart);

MODULE_DESCRIPTION("PL1029 UART Driver, v1.1.0");
MODULE_LICENSE("GPL");

