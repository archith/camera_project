
/***************************************************************
 * (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
 *
 *      Use of this software is restricted to the terms and
 *      conditions of SerComm's software license agreement.
 *
 *                        www.sercomm.com
 ****************************************************************/
#ifndef FLASH_H
#define FLASH_H
#include "memconfig.h"
#include "fl_conf.h"

/*************************************************
 *                   FLASH			 
 *   0010:0000 +----------------+ (0000:0000)  
 *             |     Loader     |      ( 56K) 	
 *             | MAC(lastest 6B)|      	
 *   0010:E000 +----------------+ (0000:E000)  
 *             |  Config Data   |      (  8K)
 *   0011:0000 +----------------+ (0001:0000) 	
 *             | Linux Kernel   |      (896K)	
 *             |                |
 *   001F:0000 +----------------+ (000F:0000)
 *             | File System    |     (3144K)
 *             |                |
 *             |                |
 *   004F:FFFF +----------------+ (003F:FFFF) (003F:FFFF)
 *************************************************/

//*********************************************************************
// Flow to control flash
// 1. InitFlashData	(optional, will be called automatically)
// 2. flash_read(), or flash_write() or flash_erase() or 
//    flash_erase_block()
// 3. flash_close()
//*********************************************************************

//#define MTD_ALLFLASH			"/dev/mtd5"
#define MTD_ALLFLASH			"/dev/mtdblock/6"
#define NUM_CHIP_SECT 71  // @@ 35
#define NUM_CHIPS 1
#define TOTAL_SECT (NUM_CHIPS*NUM_CHIP_SECT)
#define LAST_SECT   (TOTAL_SECT-1)
#define INTEL_BLOCK_0		0x2000
#define INTEL_BLOCK_64K		0x10000
#define MAC_SECT		4
#define CFG_SECT		5
#define RESERVE_SECT		6


#define FLASH_IOBASE 		BSPCONF_FLASH_BASE

#define VIDEO_RAM_SIZE	BSPCONF_FLASH_SIZE		// 4M bytes
#define	VIDEO_RAM	0x00900000		// H/W SDRAM start

#define MAC_START_ADDR	(BSPCONF_FLASH_BASE+BSPCONF_MAC_FLASH_OFFSET)	//8K
#define CFG_START_ADDR	(BSPCONF_FLASH_BASE+BSPCONF_CONF_FLASH_OFFSET)	//8K
#define KERNEL_START_ADDR	(BSPCONF_FLASH_BASE+BSPCONF_KERNEL_FLASH_OFFSET)
#define FS_START_ADDR		(BSPCONF_FLASH_BASE+BSPCONF_FS_FLASH_OFFSET)
#define FS_END_ADDR		(BSPCONF_FLASH_BASE+BSPCONF_FLASH_SIZE-1)

#define MAX_FS_LEN	(FS_END_ADDR-FS_START_ADDR+1)
#define MAX_KF_LEN	(FS_END_ADDR-KERNEL_START_ADDR+1)
#define VIDEO_RAM_END	(VIDEO_RAM+VIDEO_RAM_SIZE-1)

#define KERNEL_OFFSET	BSPCONF_KERNEL_FLASH_OFFSET	//kernel offset of upgrade bin file
#define FS_OFFSET	BSPCONF_FS_FLASH_OFFSET	//filesystem offset of upgrade bin file
#define FS_END_OFFSET	(BSPCONF_FLASH_SIZE-1)
#define CHKSUM_SIZE	8
#define PID_OFFSET	(BSPCONF_FLASH_SIZE-sizeof(struct S_PID)-CHKSUM_SIZE) // chksum=8
#define FW_SIZE		BSPCONF_FLASH_SIZE	//4M bytes
#define FLASH_SIZE	BSPCONF_FLASH_SIZE	//4M bytes

#define SET_MAX_TIMEOUT		_IOW('R',1,1)
#define	SET_MAX_RETRY		_IOW('R',2,1)
#define ACQUIRE_LOCK		_IOW('R',3,1)
#define RELEASE_LOCK		_IOW('R',4,1)
#define ERASE_BLOCK		_IOW('R',5,1)

#ifndef WORD
#define WORD  unsigned short
#endif
#ifndef BYTE
#define BYTE  unsigned char
#endif

typedef struct S_PID {
	char tag0[7];
	/* ---- PID version ---- */
	WORD wPodVersion;
	BYTE fReserved;
	BYTE fChkHw:		1;
	BYTE fCHkProduct:	1;
	BYTE fChkProtocol:	1;
	BYTE fChkFunction:	1;	
	BYTE fRsv:		4;
	/* ---- Hardware ID ---- */
	BYTE hwid[32];
	/* ---- Reserver ----*/
	WORD wRsv;	/* Reserved */
	/* ---- Product ID & mask ---- */
	WORD wProductID;	/* Motorola format */
	WORD wProductMask;	/* Motorola format */
	/* ---- Protocol ID & mask ---- */
	WORD wProtocolID;	/* Motorola format */
	WORD wProtocolMask;	/* Motorola format */
	/* ---- Function ID & mask ---- */
	WORD wFunctionID;	/* Motorola format */
	WORD wFunctionMask;	/* Motorola format */
	/* ---- F/W version ---- */
	WORD wFwVersion;	/* Motorola format */
	/* ---- Code address ---- */
	WORD wSegment;		/* Intel format */
	WORD wOffset;		/* Intel format */
	char tag1[7];
} __attribute__((packed)) PID;

//-----------------------------------------------------
// purpose: erase flash by sector 
// input: no - sector number (start from 0)
// return: 0 - ok
//         other values - error  
//-----------------------------------------------------
//int flash_erase_block(int no);


//-----------------------------------------------------
// purpose: erase flash from start_addr to end_addr
// input: start_addr - start address
//        end_addr - end address
// return: 0 - ok
//         other values - error  
//-----------------------------------------------------
//int flash_erase_range(unsigned char *start_addr, unsigned char *end_addr);


//----------------------------------------------------------------
// purpose: read data from flash
// input: addr - offset of flash base address
//	  num_bytes - number of bytes the caller wants to read
// output: dest - buffer to save data
//	   actLen - number of bytes acture readed
// return: 0 - ok
//         other values - error  
//----------------------------------------------------------------
int flash_read(unsigned char *addr,unsigned char *dest,unsigned int num_bytes, unsigned int *actLen);



//----------------------------------------------------------------
// purpose: write data to flash
// input: offset - offset of flash base address (FLASH_IOBASE)
//        src - buffer to source data
//	  num_bytes - number of bytes the caller wants to write
// outupt: actLen - number of bytes acture write
// return: 0 - ok
//         other values - error  
// Note: user must erase flash before writing !!!
//----------------------------------------------------------------
int flash_write(unsigned char *offset,unsigned char *src,unsigned int num_bytes, unsigned int * actLen);

//-----------------------------------------------------
// purpose: close flash handle
//-----------------------------------------------------
void flash_close(void);
#endif
