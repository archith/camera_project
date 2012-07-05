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

#ifndef __ADDR_H
#define __ADDR_H

#include <linux/ioctl.h>

struct  port_t {
    unsigned int    port;
    unsigned int    value;
};


#define ADDR_MAGIC     'V'

#define ADDR_WRITEL     _IOW(ADDR_MAGIC, 1, struct port_t)
#define ADDR_WRITEW     _IOW(ADDR_MAGIC, 2, struct port_t)
#define ADDR_WRITEB     _IOW(ADDR_MAGIC, 3, struct port_t)
#define ADDR_READL      _IOWR(ADDR_MAGIC, 4, struct port_t)
#define ADDR_READW      _IOWR(ADDR_MAGIC, 5, struct port_t)
#define ADDR_READB      _IOWR(ADDR_MAGIC, 6, struct port_t)
#define ADDR_PADDR      _IOWR(ADDR_MAGIC, 7, int )



#endif /* __VICAP_H */
