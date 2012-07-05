/* ------------------------------------------------------------------------- */
/* i2c-pport.c i2c-hw access  for primitive i2c par. port adapter	     */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2001    Daniel Smolik

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.		     */
/* ------------------------------------------------------------------------- */

/*
	See doc/i2c-pport for instructions on wiring to the
	parallel port connector.

	Cut & paste :-)  based on Velleman K9000 driver by Simon G. Vogl
*/


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <asm/io.h>


#define DEFAULT_BASE 0x378
static int base=0;
static unsigned char PortData = 0;

/* ----- global defines -----------------------------------------------	*/
#define DEB(x)		/* should be reasonable open, close &c. 	*/
#define DEB2(x) 	/* low level debugging - very slow 		*/
#define DEBE(x)	x	/* error messages 				*/
#define DEBINIT(x) x	/* detection status messages			*/

/* --- Convenience defines for the parallel port:			*/
#define BASE	(unsigned int)(data)
#define DATA	BASE			/* Centronics data port		*/
#define STAT	(BASE+1)		/* Centronics status port	*/
#define CTRL	(BASE+2)		/* Centronics control port	*/

/* we will use SDA  - Auto Linefeed(14)   bit 1  POUT   */
/* we will use SCL - Initialize printer(16)    BUSY bit 2*/

#define  SET_SCL    | 0x04
#define  CLR_SCL    & 0xFB




#define  SET_SDA    & 0x04
#define  CLR_SDA    | 0x02


/* ----- local functions ----------------------------------------------	*/


static void bit_pport_setscl(void *data, int state)
{
	if (state) {
		//high
		PortData = PortData SET_SCL;
	} else {
		//low
		PortData = PortData CLR_SCL; 
	}
	outb(PortData, CTRL);
}

static void bit_pport_setsda(void *data, int state)
{
	if (state) {
		
		PortData = PortData SET_SDA;
	} else {

		PortData = PortData CLR_SDA;
	}
	outb(PortData, CTRL);
} 

static int bit_pport_getscl(void *data)
{

	return ( 4 == ( (inb_p(CTRL)) & 0x04 ) );
}

static int bit_pport_getsda(void *data)
{
	return ( 0 == ( (inb_p(CTRL)) & 0x02 ) );
}

static int bit_pport_init(void)
{
	if (!request_region((base+2),1, "i2c (PPORT adapter)")) {
		return -ENODEV;	
	} else {
		/* test for PPORT adap. 	*/
	

		PortData=inb(base+2);
		PortData= (PortData SET_SDA) SET_SCL;
		outb(PortData,base+2);				

		if (!(inb(base+2) | 0x06)) {	/* SDA and SCL will be high	*/
			DEBINIT(printk("i2c-pport.o: SDA and SCL was low.\n"));
			return -ENODEV;
		} else {
		
			/*SCL high and SDA low*/
			PortData = PortData SET_SCL CLR_SDA;
			outb(PortData,base+2);	
			schedule_timeout(400);
			if ( !(inb(base+2) | 0x4) ) {
				//outb(0x04,base+2);
				DEBINIT(printk("i2c-port.o: SDA was high.\n"));
				return -ENODEV;
			}
		}
		bit_pport_setsda((void*)base,1);
		bit_pport_setscl((void*)base,1);
	}
	return 0;
}


/* ------------------------------------------------------------------------
 * Encapsulate the above functions in the correct operations structure.
 * This is only done when more than one hardware adapter is supported.
 */
static struct i2c_algo_bit_data bit_pport_data = {
	.setsda		= bit_pport_setsda,
	.setscl		= bit_pport_setscl,
	.getsda		= bit_pport_getsda,
	.getscl		= bit_pport_getscl,
	.udelay		= 40,
	.mdelay		= 80,
	.timeout	= HZ
};

static struct i2c_adapter bit_pport_ops = {
	.owner		= THIS_MODULE,
	.name		= "Primitive Parallel port adaptor",
	.id		= I2C_HW_B_PPORT,
	.algo_data	= &bit_pport_data,
};

int __init i2c_bitpport_init(void)
{
	printk("i2c-pport.o: i2c Primitive parallel port adapter module version %s (%s)\n", I2C_VERSION, I2C_DATE);

	if (base==0) {
		/* probe some values */
		base=DEFAULT_BASE;
		bit_pport_data.data=(void*)DEFAULT_BASE;
		if (bit_pport_init()==0) {
			if(i2c_bit_add_bus(&bit_pport_ops) < 0)
				return -ENODEV;
		} else {
			return -ENODEV;
		}
	} else {
		bit_pport_data.data=(void*)base;
		if (bit_pport_init()==0) {
			if(i2c_bit_add_bus(&bit_pport_ops) < 0)
				return -ENODEV;
		} else {
			return -ENODEV;
		}
	}
	printk("i2c-pport.o: found device at %#x.\n",base);
	return 0;
}

static void __exit i2c_bitpport_exit(void)
{
	i2c_bit_del_bus(&bit_pport_ops);
	release_region((base+2),1);
}

EXPORT_NO_SYMBOLS;

MODULE_AUTHOR("Daniel Smolik <marvin@sitour.cz>");
MODULE_DESCRIPTION("I2C-Bus adapter routines for Primitive parallel port adapter");
MODULE_LICENSE("GPL");

MODULE_PARM(base, "i");

module_init(i2c_bitpport_init);
module_exit(i2c_bitpport_exit);
