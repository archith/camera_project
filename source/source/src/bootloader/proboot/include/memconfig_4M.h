/* asm/arch/memconfig.h */

#ifndef __ASM_ARCH_MEMCONFIG_H_
#define __ASM_ARCH_MEMCONFIG_H_

/*************************************************
 *                   FLASH
 *   1980:0000 +----------------+ (0000:0000)
 *             |     Loader     |      ( 32K)
 *   1980:8000 +----------------+ (0000:8000)
 *             | HTTPS CA Data  |      ( 16K)
 *   1980:C000 +----------------+ (0000:C000)
 *             | MAC(lastest 6B)|      (  8K)
 *   1980:E000 +----------------+ (0000:E000)
 *             |  Config Data   |      (  8K)
 *   1981:0000 +----------------+ (0001:0000)
 *             | Linux Kernel   |      (768K)
 *             |                |
 *   198D:0000 +----------------+ (000D:0000)
 *             | File System    |     (3264K)
 *             |                |
 *             |                |
 *   19BF:FFFF +----------------+ (003F:FFFF)
 *************************************************/

#define BSPCONF_FLASH_BASE 0x19800000
#define BSPCONF_FLASH_SIZE 0x00400000
#define FLASH_BASE BSPCONF_FLASH_BASE
#define FLASH_SIZE BSPCONF_FLASH_SIZE

#define BSPCONF_SDRAM_BASE 0x00000000 /* ?? */
#define BSPCONF_SDRAM_SIZE 0x02000000 /* 32MB */

/* FLASH Memory Map */
#define BSPCONF_FLASH_PARTITIONS        6	/* LDR+MAC+CONF+HTTPS+KNL+FS */

#define BSPCONF_BTLDR_FLASH_OFFSET      0x00000000
#define BSPCONF_BTLDR_FLASH_SIZE        0x00008000 /* 32KB Loader*/

#define BSPCONF_HTTPS_FLASH_OFFSET      0x00008000
#define BSPCONF_HTTPS_FLASH_SIZE        0x00004000 /* 16KB HTTPS CA Data*/

#define BSPCONF_MAC_FLASH_OFFSET	0x0000C000
#define BSPCONF_MAC_FLASH_SIZE          0x00002000 /* 8KB MAC(lastest 6B)*/
#define BSPCONF_MAC6BYTE_OFFSET        (BSPCONF_MAC_FLASH_OFFSET+BSPCONF_MAC_FLASH_SIZE-6)

#define BSPCONF_CONF_FLASH_OFFSET       0x0000E000
#define BSPCONF_CONF_FLASH_SIZE         0x00002000 /* 8KB Config Data*/

#define BSPCONF_KERNEL_FLASH_OFFSET     0x00010000
#define BSPCONF_KERNEL_FLASH_SIZE       0x000C0000 /* 768KB */

#define BSPCONF_FS_FLASH_OFFSET         0x000D0000
#define BSPCONF_FS_FLASH_SIZE           0x00330000 /* 3264KB */


#endif /* __ASM_ARCH_MEMCONFIG_H_ */
