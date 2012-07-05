/* asm/arch/memconfig.h */

#ifndef __ASM_ARCH_MEMCONFIG_H_
#define __ASM_ARCH_MEMCONFIG_H_

/*************************************************
 *     WVC2300 8MB flash memory map
 -------------------------------------------------
 *                   FLASH			 
 *   1980:0000 +----------------+ (0000:0000)  
 *             |     Loader     |      ( 64K) 	
 *             | 	        |      	
 *   1981:0000 +----------------+ (0001:0000)  
 *             | MAC(lastest 6B)|      ( 64K)
 * 	       |serial(12Bytes)	|
 *   1982:0000 +----------------+ (0002:0000)  
 *             |  Config Data   |      ( 64K)
 * 	       |		|
 *   1983:0000 +----------------+ (0003:0000)
 *             | LOGO Image     |      ( 64K) 
 *             |                |
 *   1984:0000 +----------------+ (0004:0000)	    	
 *	       | HTTPS CA	|      ( 64K)
 *	       |		|		
 *   1985:0000 +----------------+ (0005:0000)   
 *	       | Root CA	|      ( 64K)	
 *	       |		|
 *   1986:0000 +----------------+ (0006:0000)
 *	       | User CA	|      ( 64K)
 *	       |		|
 *   1987:0000 +----------------+ (0007:0000) 	
 *             | Linux Kernel   |      (832K)	
 *             |                |
 *   1994:0000 +----------------+ (0014:0000)
 *             | File System    |     (6912K)
 *             |                |
 *   19FF:FFFF +----------------+ (007F:FFFF)
 *************************************************/
#define BSPCONF_FLASH_BASE 0x19800000
#define BSPCONF_FLASH_SIZE 0x00800000
#define FLASH_BASE BSPCONF_FLASH_BASE
#define FLASH_SIZE BSPCONF_FLASH_SIZE

#define BSPCONF_SDRAM_BASE 0x00000000 /* ?? */
#define BSPCONF_SDRAM_SIZE 0x02000000 /* 32MB */

/* FLASH Memory Map */
#define BSPCONF_FLASH_PARTITIONS        9	/* LDR+MAC+CONF+HTTPS+KNL+FS */

#define BSPCONF_BTLDR_FLASH_OFFSET      0x00000000
#define BSPCONF_BTLDR_FLASH_SIZE        0x00010000 /* 64KB Loader*/

#define BSPCONF_MAC_FLASH_OFFSET	(BSPCONF_BTLDR_FLASH_OFFSET+BSPCONF_BTLDR_FLASH_SIZE)
#define BSPCONF_MAC_FLASH_SIZE		0x00010000 /* 64KB MAC(lastest 6B)*/
#define BSPCONF_SERIAL_OFFSET		(BSPCONF_MAC_FLASH_OFFSET)
#define BSPCONF_SERIAL_SIZE			0x20
#define BSPCONF_WLAN_DOMAIN_OFFSET	(BSPCONF_SERIAL_OFFSET+BSPCONF_SERIAL_SIZE)
#define BSPCONF_WLAN_DOMAIN_SIZE	0x1
#define BSPCONF_LANGUAGE_OFFSET		(BSPCONF_WLAN_DOMAIN_OFFSET+BSPCONF_WLAN_DOMAIN_SIZE + 0xF)
#define BSPCONF_LANGUAGE_SIZE		0x4
#define BSPCONF_CISCO_PID_OFFSET	(BSPCONF_LANGUAGE_OFFSET+BSPCONF_LANGUAGE_SIZE + 0xC)
#define BSPCONF_CISCO_PID_SIZE		0x12
#define BSPCONF_CISCO_VID_OFFSET	(BSPCONF_CISCO_PID_OFFSET+BSPCONF_CISCO_PID_SIZE + 0xE)
#define BSPCONF_CISCO_VID_SIZE		0x3
#define BSPCONF_MAC6BYTE_OFFSET		(BSPCONF_MAC_FLASH_OFFSET+BSPCONF_MAC_FLASH_SIZE-6)

#define BSPCONF_CONF_FLASH_OFFSET       (BSPCONF_MAC_FLASH_OFFSET+BSPCONF_MAC_FLASH_SIZE)
#define BSPCONF_CONF_FLASH_SIZE         0x00010000 /* 64KB Config Data*/

#define BSPCONF_LOGO_IMAGE_FLASH_OFFSET	(BSPCONF_CONF_FLASH_OFFSET+BSPCONF_CONF_FLASH_SIZE)
#define BSPCONF_LOGO_IMAGE_FLASH_SIZE	0x00010000 /* 64KB */

#define BSPCONF_HTTP_CA_FLASH_OFFSET	(BSPCONF_LOGO_IMAGE_FLASH_OFFSET+BSPCONF_LOGO_IMAGE_FLASH_SIZE)
#define BSPCONF_HTTP_CA_FLASH_SIZE	0x00010000 /* 64KB */

#define BSPCONF_ROOT_CA_FLASH_OFFSET	(BSPCONF_HTTP_CA_FLASH_OFFSET+BSPCONF_HTTP_CA_FLASH_SIZE)
#define BSPCONF_ROOT_CA_FLASH_SIZE	0x00010000 /* 64KB */

#define BSPCONF_USER_CA_FLASH_OFFSET	(BSPCONF_ROOT_CA_FLASH_OFFSET+BSPCONF_ROOT_CA_FLASH_SIZE)
#define BSPCONF_USER_CA_FLASH_SIZE	0x00010000 /* 64KB */

#define BSPCONF_KERNEL_FLASH_OFFSET     (BSPCONF_USER_CA_FLASH_OFFSET+BSPCONF_USER_CA_FLASH_SIZE)
#define BSPCONF_KERNEL_FLASH_SIZE       0x000D0000 /* 832KB */

#define BSPCONF_FS_FLASH_OFFSET         (BSPCONF_KERNEL_FLASH_OFFSET+BSPCONF_KERNEL_FLASH_SIZE)
#define BSPCONF_FS_FLASH_SIZE           0x006C0000 /* 6912KB */




#endif /* __ASM_ARCH_MEMCONFIG_H_ */
