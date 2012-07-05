/*
 *  linux/include/asm-arm/arch-pl1097/serial.h
 */
#ifndef __ASM_ARCH_SERIAL_H
#define __ASM_ARCH_SERIAL_H

#include <asm/irq.h>
#include <asm/hardware.h>

#define RS_TABLE_SIZE       1

#define PL_COM_FLAGS        (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST)

#define PORT_PL_SIO         14

#define STD_SERIAL_PORT_DEFNS \
    { baud_base: 0,     /* ttyS0 */ \
      irq: IRQ_PL_CONSOLE,          \
      flags: PL_COM_FLAGS,          \
      type: PORT_PL_SIO,            \
      xmit_fifo_size: 4,            \
      iomem_base: (u8 *)rCONSOLE_BASE, \
      iomem_reg_shift: 0,           \
      io_type: SERIAL_IO_MEM },

#define EXTRA_SERIAL_PORT_DEFNS

#define BASE_BAUD 3686400


#endif
