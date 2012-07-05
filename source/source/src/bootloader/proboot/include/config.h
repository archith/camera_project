#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "version.h"

#define ARG_CNT	16
#define ARG_LEN	2048

#define CPU	"ARM FA526"
#define STR_CREDIT "We fight for the honor of our company.\n"\
                   "Prolific bootloader - Proboot - Professional bootloader\n"\
                   "by Jun Chen, Hugo Leu, Jedy Wei, CC Yen 2003-2006 \n"

/*
 * STACK init in bootsect.s
 */
#define STACK_ADDR	0x400000
#define STACK_SIZE	0x40000

/*
 * MMU place the page tables 16K from 0000_4000.
 */
#define PAGETBL_ADDR            0x00004000
#define PAGETBL_SIZE            0x00004000
#define RAMMAP_ADDR             0
#define RAMMAP_SIZE             (16*1024*1024)
#define IOMAP_ADDR              0x18000000
#define IOMAP_SIZE              (0x20000000-0x18000000)
#define NORMAP_ADDR             0x19800000
#define NORMAP_SIZE             (4*1024*1024)

/* parameter transfer to kernel */
#define TAG_ADDR        0x2000
#define MAX_TAG_SIZE    256
#define TAG_MAXSIZE     (0x4000-0x2000)
#define PARAM_MAXSIZE   (TAG_MAXSIZE-256)

#endif // __CONFIG_H__
