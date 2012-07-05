#ifndef CROSS_H
#define CROSS_H

//#define _LAN_DRV_DM9102_
//#define _LAN_DRV_IP100A_

#include <stdio.h>
#include <string.h>
#include <pl_flash.h>
#include <memconfig.h>
#include <io.h>
#include <nor.h>
#include <net.h>
//lan
#define util_printf printf
// Predefines for all the network API
#ifdef _LAN_DRV_DM9102_
#include <dm9102.h>
#define Lan_Receive				secdmfe_receive
#define Lan_phy_SetMACAddr		secdmfe_setmac
#define Lan_Transmit			secdmfe_transmit
#define Lan_phy_init			secdmfe_open
#endif
#ifdef _LAN_DRV_IP100A_
#include <ip100a.h>
#define Lan_Receive 			ip100a_sd_receive
#define Lan_phy_SetMACAddr		ip100a_sd_setmac
#define Lan_Transmit			ip100a_sd_transmit
#define Lan_phy_init			ip100a_sd_open
#endif

#ifdef _WVC2300_
	#define MODEL_PID	0x59,0x43,0x33
#endif
#ifdef _PVC2300_
	#define MODEL_PID	0x59,0x43,0x51
#endif
//LED
#define GPIO_REG_BASE		0x194c0000
#define GPIO_REG_OE			(GPIO_REG_BASE + 0x54)
#define GPIO_REG_LED		(GPIO_REG_BASE + 0x56)
#define GPIO_REG_OD			(GPIO_REG_BASE + 0x58)
#define GPIO_REG_ID			(GPIO_REG_BASE + 0x5C)
#if 1		// Here for SerComm Platform
#define PUSHBUTTON			4
#define LED1				0
#define LED2				1
#define LED3				2
#define READ_PUSHBUTTON()	(~(readw(GPIO_REG_ID)) & (1<<PUSHBUTTON))
#else		// Here for EVM board
#define PUSHBUTTON			7
#define LED1				8
#define LED2				9
#define LED3				10
#define READ_PUSHBUTTON()	((readw(GPIO_REG_ID)) & (1<<PUSHBUTTON))
#endif
#define SetLED(x) 			writew(readw(GPIO_REG_LED) | (1<<x), GPIO_REG_LED)
#define ClrLED(x)			writew(readw(GPIO_REG_LED) & ~(1<<x), GPIO_REG_LED)
#define INIT_LED()			writew(readw(GPIO_REG_OE) | (1<<LED1) | (1<<LED2) | (1<<LED3), GPIO_REG_OE);\
					writew(readw(GPIO_REG_OD) | (1<<LED1) | (1<<LED2) | (1<<LED3), GPIO_REG_OD)

//Reboot
#define	REBOOT()			writew(0xFFFF, 0x1B00000E);

#define SC_DBG					printf
#define u_memset 				memset
#define u_memcpy				memcpy
#define malloc					sc_malloc

#define Lan_phy_GetMACAddr(a)	memcpy(a, (void*)(BSPCONF_FLASH_BASE+BSPCONF_MAC6BYTE_OFFSET), 6)   

//FLASH functions
#define flash_read(a, b, c)		nor2mem(a, b, c)
//#define block_erase(a)				pl_flash_erase_range((unsigned int)a,(unsigned int)a)
#define flash_write_v1(a,b,c)		pl_flash_write_v1(a, b, c)
#define flash_erase_v1()		pl_flash_erase_range(BSPCONF_CONF_FLASH_OFFSET,FLASH_SIZE-1)
#define flash_erase_range(a,b)		pl_flash_erase_range(a,b)


#define virt_to_bus				virt_to_phys

#define MALLOC_START			(BSPCONF_SDRAM_BASE+0x800000)
#define MALLOC_SIZE				0x200000
#define EMPTY_SDRAM_START		(MALLOC_START+MALLOC_SIZE)
#define EMPTY_SDRAM_SIZE		0x800000

#define DEFAUL_IP_1	192
#define DEFAUL_IP_2	168
#define DEFAUL_IP_3	1
#define DEFAUL_IP_4	99
#endif

