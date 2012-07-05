#ifndef __IO_H__
#define __IO_H__

/*
 * Memory segments (32bit kernel mode addresses)
 */

#define KUSEG                   0x00000000
#define KSEG0                   0x00000000	// Cacheable
#define KSEG1                   0x00000000	// Uncacheable
#define KSEG2                   0x00000000

/*
 * Returns the physical address of a KSEG0/KSEG1 address
 */

#define PHYSADDR(address)	((unsigned int)(address) & 0x1fffffff)

#define virt_to_phys(address)	PHYSADDR(address)

/*
 * Map an address to a certain kernel segment
 */

#define KSEG0ADDR(address)	((unsigned int)PHYSADDR(address) | KSEG0)
#define KSEG1ADDR(address)	((unsigned int)PHYSADDR(address) | KSEG1)
#define KSEG2ADDR(address)	((unsigned int)PHYSADDR(address) | KSEG2)

#define phys_to_virt(address)	(void *)KSEG0ADDR(address)

/* 
 * Access 32/16/8 bit Non-Cache-able (KSEG1)
 */

#define readb(addr)		(*(volatile unsigned char *)(addr))
#define readw(addr)		(*(volatile unsigned short *)(addr))
#define readl(addr)		(*(volatile unsigned int *)(addr))

#define writeb(b,addr)		((*(volatile unsigned char *)(addr)) = (b))
#define writew(b,addr)		((*(volatile unsigned short *)(addr)) = (b))
#define writel(b,addr)		((*(volatile unsigned int *)(addr)) = (b))

#endif // __IO_H__
