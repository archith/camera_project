/*********************************************************************
 * Filename:      pl_uart.h
 * Version:       0.1
 * Description:   PL-1061B UART driver header
 * Created at:    Sep 18 2003
 ********************************************************************/

#include <linux/config.h>


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

// #define PL_UART_BASE 0xBAC80000      // For 106x

#define PL_UART_BASE    0xd9600000
#define PL_UART_1_BASE  0xd95c0000      // 2007/02/12
#define PL_GPIO_BASE    0xd94c0000

// GPIO Registers
#define GPIO_ENB        (PL_GPIO_BASE + 0x54)
#define GPIO_DO         (PL_GPIO_BASE + 0x56)  /* Data Ouput */
#define GPIO_OE         (PL_GPIO_BASE + 0x58)  /* 1:Output - 0:Input Selection */
#define GPIO_DI         (PL_GPIO_BASE + 0x5c)  /* Data input */
#define GPIO_KEY_ENB    (PL_GPIO_BASE + 0x78)  /* Keypad enable */
#define GPIO_KEY_POL    (PL_GPIO_BASE + 0x7a)  /* Keypad input polarity select 0: rasing edge 1: falling edge */
#define GPIO_RSTN       (PL_GPIO_BASE + 0x7e)
#define RSTN_KEY        (1L << (30-16))
#define RSTN_PWM        (1L << (31-16))
#define GPIO_PF0_PU     (PL_GPIO_BASE + 0x64)
#define GPIO_PF0_PD     (PL_GPIO_BASE + 0x66)

#define GPIO_IO_SEL     (PL_GPIO_BASE + 0x103)  // 2007/02/15

// Pins
#define PIN_DTR       0x10
#define PIN_DSR       0x20
#define PIN_DCD       0x40
#define PIN_RI        0x80


#define PL_COM_FLAGS        (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST)

#if     0
#define UART_TABLE_SIZE     1
#define UART_PORT_DFNS \
 { \
  baud_base: 1, \
  irq : IRQ_PL_UART_1, \
  flags : PL_COM_FLAGS, \
  type : PORT_PL_UART_1, \
  xmit_fifo_size : 2, \
  iomem_base : (u8 *)PL_UART_1_BASE,\
  iomem_reg_shift : 0, \
  io_type : SERIAL_IO_MEM \
 },
#endif

#define DMA_OUT_BUF_SIZE  PAGE_SIZE
#define DMA_IN_BUF_SIZE   PAGE_SIZE

#define MAX_DMA_OUT_COUNT  4095
#define MAX_DMA_IN_COUNT  4095

#define DMA_IN_SIZE     4095
#define DMA_IN_WATERMARK  (DMA_IN_SIZE * 7 / 8)

#define ACIO_RTS 0
#define ACIO_CTS 1

#if 0
#define dbg_out(format, arg...) \
do { \
  udelay(10); \
 printk( format, ## arg); \
} while (0)
#else
#define dbg_out(format, arg...)
#endif

