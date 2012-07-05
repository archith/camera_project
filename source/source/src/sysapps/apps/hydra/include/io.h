/* $Id: io.h,v 1.3 2006/04/04 03:10:23 jedy Exp $
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 Waldorf GmbH
 * Copyright (C) 1994 - 2000 Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 */

#ifndef _USER_IO_H
#define _USER_IO_H

extern int io_init(void);
extern int io_release(void);
extern unsigned long io_mmappaddr(void);

#ifdef CONFIG_PLIO_BY_IOCTL

extern unsigned readb(unsigned addr);
extern unsigned readw(unsigned addr);
extern unsigned readl(unsigned addr);

extern int writeb(unsigned value, unsigned addr);
extern int writew(unsigned value, unsigned addr);
extern int writel(unsigned value, unsigned addr);
#else

extern unsigned long iooffset;
#define readb(addr) (*(volatile unsigned char *)(addr-iooffset))
#define readw(addr) (*(volatile unsigned short *)(addr-iooffset))
#define readl(addr) (*(volatile unsigned int *)(addr-iooffset))

#define writeb(b,addr) (*(volatile unsigned char *)(addr-iooffset)) = (b)
#define writew(b,addr) (*(volatile unsigned short *)(addr-iooffset)) = (b)
#define writel(b,addr) (*(volatile unsigned int *)(addr-iooffset)) = (b)
#endif


#endif /* _USER_IO_H */
