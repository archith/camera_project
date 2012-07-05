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
#ifndef __PL_SYSTEM_H
#define __PL_SYSTEM_H

/* Level 2 Interrupt controller registers */
#define PL_INT_BASE         (rINTERRUPT_CONTROLLER_BASE)
#define PL_INT_MASK         (PL_INT_BASE + 0)
#define PL_INT_STATUS       (PL_INT_BASE + 2)
#define PL_INT_VECTOR       (PL_INT_BASE + 4)

/* DMA controller registers */
#define PL_DMA_BASE         (rDMAC0_BASE)
#define PL_DMA_INT_MASK     (PL_DMA_BASE + 0x100)
#define PL_DMA_INT_STATUS   (PL_DMA_BASE + 0x102)
#define PL_DMA_INT_VECTOR   (PL_DMA_BASE + 0x104)

/* Console registers */
#define PL_CONSOLE_BASE     (rCONSOLE_BASE)

/* Timer and RTC registers */
#define PL_TIMER_BASE       (rTIMER_BASE)
#define PL_TIMER_STI_P      (PL_TIMER_BASE + 0)
#define PL_TIMER_STI_ENA    (PL_TIMER_BASE + 1)
#define PL_TIMER_STI_CLR    (PL_TIMER_BASE + 2)
#define PL_TIMER_WDI_CLR    (PL_TIMER_BASE + 3)

/* PCI Configuartion Registers */
#define PL_PCI_IO_BASE      (rPCI_IOPORT_BASE)
#define PL_PCI0_CFGADDR_OFS (PL_PCI_IO_BASE + 0xCF8)
#define PL_PCI0_CFGDATA_OFS (PL_PCI_IO_BASE + 0xCFC)

/* USB Registers */
#define PL_USB11_HOST_BASE  (rUSB11_HOST_BASE)

/* Memory Configuration Registers */
#define PL_MEM_BASE         (rCMC_BASE)
#define PL_MEM_SIZE         (PL_MEM_BASE)

/* Clock Configuration Registers */
#define PL_SYS_BASE         (rSYS_MANAGE_BASE)
#define PL_CLK_CFG          (PL_SYS_BASE)
#define PL_SHUTDOWN_INFO    (PL_SYS_BASE + 0x04)
#define PL_OPMODE_SW        (PL_SYS_BASE + 0x08)
#define PL_OPMODE_SW2       (PL_SYS_BASE + 0x0C)
#define PL_CLK_CFG2         (PL_SYS_BASE + 0x10)
#define PL_CLK_CFG3         (PL_SYS_BASE + 0x14)

#define PL_CLK_NAND         (PL_SYS_BASE + 0x410)
#define PL_CLK_I2C          (PL_SYS_BASE + 0x411)
#define PL_CLK_AC97         (PL_SYS_BASE + 0x412)
#define PL_CLK_GPIO         (PL_SYS_BASE + 0x413)
#define PL_CLK_IDE1         (PL_SYS_BASE + 0x414)
#define PL_CLK_SD           (PL_SYS_BASE + 0x415)
#define PL_CLK_UART_1       (PL_SYS_BASE + 0x417)
#define PL_CLK_UART         (PL_SYS_BASE + 0x418)
#define PL_CLK_MS           (PL_SYS_BASE + 0x419)
#define PL_CLK_JMP4         (PL_SYS_BASE + 0x41a)
#define PL_CLK_SPI          (PL_SYS_BASE + 0x41c)
#define PL_CLK_AES          (PL_SYS_BASE + 0x41d)
#define PL_CLK_NOR          (PL_SYS_BASE + 0x41e)


#endif /* __PL_SYSTEM_H */
