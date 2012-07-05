/*
 * linux/include/asm/arch-pl1029/io.h
 *
 * Copyright (C) 1997-1999 Russell King
 *
 * Modifications:
 *  06-12-1997	RMK	Created.
 *  07-04-1999	RMK	Major cleanup
 *  02-19-2001  gjm     Leveraged for armnommu/dsc21
 *  07-15-2002  Jim Lee Port to armnommu/pl1071
 */
#ifndef __ASM_ARM_ARCH_IO_H
#define __ASM_ARM_ARCH_IO_H

#include "pl1029.h"

/*
 * kernel/resource.c uses this to initialize the global ioport_resource struct
 * which is used in all calls to request_resource(), allocate_resource(), etc.
 * --gmcnutt
 */
#define IO_SPACE_LIMIT 0xffffffff

/*
 * If we define __io then asm/io.h will take care of most of the inb & friends
 * macros. It still leaves us some 16bit macros to deal with ourselves, though.
 * We don't have PCI or ISA on the dsc21 so I dropped __mem_pci & __mem_isa.
 * --gmcnutt
 */
#if 0
/* 
 * needless to add the offset here, PCI resource allocation is done in
 * earlier stage.
 */
#define __io(port)          ((port) + rPCI_IOPORT_BASE)
#else
#define __io(port)          (port)
#endif
#define __mem_pci(addr)     (addr)
#define __mem_isa(addr)     (addr)

/*
 * Generic virtual read/werite
 */
#define __arch_getw(a)      (*(volatile unsigned short *)(a))
#define __arch_putw(v,a)    (*(volatile unsigned short *)(a) = (v))

#define iomem_valid_addr(iomem,sz)	(1)
#define iomem_to_phys(iomem)		(iomem)

#if 0
/*
 * Defining these two gives us ioremap for free. See asm/io.h.
 * --gmcnutt
 */
/* avoid converting IO address to virtual address twice, by Vincent 931108 */
#define iomem_valid_addr(iomem,sz)	\
	( !((iomem & VIRT_IO_BASE) == VIRT_IO_BASE) )
#endif



#endif /* __ASM_ARM_ARCH_IO_H */

