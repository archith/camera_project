/* pl1060.h
 * PL1060 Hardware Header File
 * 2002/01/08
 * Jim Lee
 */

#ifndef __PL1060_H
#define __PL1060_H
#include "CandMCx.h"

/* Build-in Interrupt Controller */
#define INT_MASK1_REG      0xBB000100
#define INT_MASK2_REG      0xBB000101
#define INT_ST1_REG        0XBB000102
#define INT_ST2_REG        0xBB000103
#define INT_VECTOR_REG     0xBB000104

/* System Timer interface */
#define STI_P_REG          0xBB000200
#define STI_ENA_REG        0xBB000201
#define STI_CLR_REG        0xBB000202
#define WDI_CLR_REG        0xBB000203

#define SYS_TICK_00        0x00       /* 1/16 sec = 62.5 ms */
#define SYS_TICK_01        0x01       /* 1/32 sec = 31.25 ms */
#define SYS_TICK_02        0x02       /* 1/64 sec = 15.625 ms */
#define SYS_TICK_03        0x03       /* 1/128 sec = 7.8125 ms */

/* Real Time Clock Interface */
#define RTC_REG            0xBB000204

/* EEPROM interface */
#define EE_TWS_STT_REG     0xBB000303
#define EE_TWS_DIO_REG     0xBB000302
#define EE_TWS_CMD_REG     0xBB000301
#define EE_TWS_ADR_REG     0xBB000300


/* Build-in Console */
#define CON_STATUS_REG     0xBB000400
#define CON_DATA_REG       0xBB000401

/* LED Display Register */
#define CON_SYS_REG        0xBB000402
#define CON_RST_REG        0xBB000403

/* Cache & Memory Controller */
#define MEM_ADDR_MAP_REG   0xBC000000
#define MEM_MODE_REG       0xBC000004
#define MEM_CLK_REG        0xBC000008
#define SDRAM_CFG_REG      0xBC00000C
#define FLUSH_L2_TAG_REG   0xBC000010

#define L2_DATA_START_ADDR 0xBD000000
#define L2_DATA_SIZE       0x00004000
#define L2_TAG_START_ADDR  0xBD004000
#define L2_TAG_SIZE        0x00008000

#define MEM_ADDR_MAP_VALUE ( FB_SIZE_CONFIG + KERNEL_SIZE_CONFIG + RAM_SIZE_CONFIG )
#define MEM_ADDR_MAP_VALUES 0x000008DC


#define MEM_MODE_VALUE     0xC12A870C
#define MEM_CLK_VALUE      0x2a00ff17
#define SDRAM_CFG_VALUE    0x00000237
#define FLUSH_L2_TAG_VALUE 0x00000001


/* PL1060 Hardware Dependent Marco Definition */
/* #define outb(address,value)	(*((volatile unsigned long*)(address)))=(value) */
/* #define inb(address)            (*((volatile unsigned long*)(address)))      */
#define htol(value) (((value & 0x00ff) << 24) | ((value & 0xff00) << 8) | ((value >> 8 ) & 0xff00) | ((value >> 24) & 0x00ff))


/* LED symbol for Debug */
#define SYS_BOARD_INIT        0x01
#define SYS_BOARD_END         0x02
#define SYS_BOARD_END2        0x03
#define SYS_DECOMPRESS_KERNEL 0x04
#define SYS_DECOMPRESS_MID    0x05
#define SYS_DECOMPRESS_OK     0x06
#define SYS_KERNEL_ENTRY      0x07

/* Console misc definition */
#define OP_TXRDY              1
#define OP_TX                 2
#define OP_RXRDY              3
#define OP_RX                 4

#define US_RXRDY              0X80
#define US_TXRDY              0x04

#define DELAY_CNT             1000

/* PCI MISC */
#define PL1060_PCI_BASE       0xbb800cf8
#define PL1060_PCI_ADDR_OFS   0x00
#define PL1060_PCI_DATA_OFS   0x04

#define PL1060_PCI_EXT_MEM_MAP  0x1a000000
#define PL1060_PCI_MEM_MAP_SIZE 0x400000
#define PL1060_PCI_MEM_MAP_PER_BA  0x10000


#define PL1060_PCI_EXT_IO_MAP   0x1a000000
#define PL1060_PCI_IO_MAP_SIZE  0x400000

#define PL1060_PCI_DEVFN(slot,func)	(slot == 4) ? (( 4 << 3) | (func & 0x07)) :((slot+4) << 3  | (func & 0x07))


/* LCD misc */
#define PL1060_LCD_BASE            0xbb400000
#define PL1060_LCD_CONTROL         PL1060_LCD_BASE
#define PL1060_LCD_BASE_ADDR       (PL1060_LCD_BASE + 0x04)
#define PL1060_LCD_CURSOR_COORD    (PL1060_LCD_BASE + 0X08)
#define PL1060_LCD_MISC_CONTROL    (PL1060_LCD_BASE + 0x0c)
#define PL1060_LCD_CURSOR_BLUE     (PL1060_LCD_BASE + 0x10)
#define PL1060_LCD_CURSOR_GREEN    (PL1060_LCD_BASE + 0x14)
#define PL1060_LCD_CURSOR_RED      (PL1060_LCD_BASE + 0x18)

#define PL1060_LCD_VSYNC           (PL1060_LCD_BASE + 0x24)
#define PL1060_LCD_HSYNC           (PL1060_LCD_BASE + 0x28)
#define PL1060_LCD_VH_PARA         (PL1060_LCD_BASE + 0x2c)
#define PL1060_LCD_RAM_SEL         (PL1060_LCD_BASE + 0x30)

#define PL1060_LCD_STN_PTR0        (PL1060_LCD_BASE + 0x40)
#define PL1060_LCD_STN_PTR1        (PL1060_LCD_BASE + 0x44)

#define PL1060_LCD_STN_COLOR0      (PL1060_LCD_BASE + 0x50)
#define PL1060_LCD_STN_COLOR1      (PL1060_LCD_BASE + 0x54)
#define PL1060_LCD_STN_COLOR2      (PL1060_LCD_BASE + 0x58)
#define PL1060_LCD_STN_COLOR3      (PL1060_LCD_BASE + 0x5a)
#define PL1060_LCD_STN_COLOR4      (PL1060_LCD_BASE + 0x60)
#define PL1060_LCD_STN_COLOR5      (PL1060_LCD_BASE + 0x64)
#define PL1060_LCD_STN_COLOR6      (PL1060_LCD_BASE + 0x68)

#define PL1060_LCD_STATUS_INT      (PL1060_LCD_BASE + 0x70)
#define PL1060_LCD_BUFFER          (PL1060_LCD_BASE + 0x8000)


#endif
