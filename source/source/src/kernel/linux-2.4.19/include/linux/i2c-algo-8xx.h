/* ------------------------------------------------------------------------- */
/* i2c-algo-8xx.h i2c driver algorithms for MPX8XX CPM			     */
/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/* ------------------------------------------------------------------------- */

/* $Id: i2c-algo-8xx.h,v 1.1.1.1 2006/03/13 10:29:24 jedy Exp $ */

#ifndef _LINUX_I2C_ALGO_8XX_H
#define _LINUX_I2C_ALGO_8XX_H

#include "asm/commproc.h"

struct i2c_algo_8xx_data {
	uint dp_addr;
	int reloc;
	volatile i2c8xx_t *i2c;
	volatile iic_t	*iip;
	volatile cpm8xx_t *cp;

	int	(*setisr) (int irq,
			   void (*func)(void *, void *),
			   void *data);

	u_char	temp[513];
};

int i2c_8xx_add_bus(struct i2c_adapter *);
int i2c_8xx_del_bus(struct i2c_adapter *);

#endif /* _LINUX_I2C_ALGO_8XX_H */
