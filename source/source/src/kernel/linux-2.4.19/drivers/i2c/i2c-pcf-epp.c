/* ------------------------------------------------------------------------- */
/* i2c-pcf-epp.c i2c-hw access for PCF8584 style EPP parallel port adapters  */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 1998-99 Hans Berglund

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

/* With some changes from Ryosuke Tajima <rosk@jsk.t.u-tokyo.ac.jp> */

#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/parport.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-pcf.h>
#include <asm/irq.h>
#include <asm/io.h>


struct  i2c_pcf_epp {
  int pe_base;
  int pe_irq;
  int pe_clock;
  int pe_own;
} ;

#define DEFAULT_BASE 0x378
#define DEFAULT_IRQ      7
#define DEFAULT_CLOCK 0x1c
#define DEFAULT_OWN   0x55

static int base  = 0;
static int irq   = 0;
static int clock = 0;
static int own   = 0;
static int i2c_debug=0;
static struct i2c_pcf_epp gpe;
static wait_queue_head_t pcf_wait;
static int pcf_pending;
static spinlock_t irq_driver_lock = SPIN_LOCK_UNLOCKED;

/* ----- global defines -----------------------------------------------	*/
#define DEB(x)	if (i2c_debug>=1) x
#define DEB2(x) if (i2c_debug>=2) x
#define DEB3(x) if (i2c_debug>=3) x
#define DEBE(x)	x	/* error messages 				*/

/* --- Convenience defines for the EPP/SPP port:			*/
#define BASE	((struct i2c_pcf_epp *)(data))->pe_base
// #define DATA	BASE			/* SPP data port */
#define STAT	(BASE+1)		/* SPP status port */
#define CTRL	(BASE+2)		/* SPP control port */
#define EADD	(BASE+3)		/* EPP address port */
#define EDAT	(BASE+4)		/* EPP data port */

/* ----- local functions ----------------------------------------------	*/

static void pcf_epp_setbyte(void *data, int ctl, int val)
{
  if (ctl) {
    if (gpe.pe_irq > 0) {
      DEB3(printk(KERN_DEBUG "i2c-pcf-epp.o: Write control 0x%x\n",
		  val|I2C_PCF_ENI));
      // set A0 pin HIGH
      outb(inb(CTRL) | PARPORT_CONTROL_INIT, CTRL);
      // DEB3(printk(KERN_DEBUG "i2c-pcf-epp.o: CTRL port = 0x%x\n", inb(CTRL)));
      // DEB3(printk(KERN_DEBUG "i2c-pcf-epp.o: STAT port = 0x%x\n", inb(STAT)));
      
      // EPP write data cycle
      outb(val | I2C_PCF_ENI, EDAT);
    } else {
      DEB3(printk(KERN_DEBUG "i2c-pcf-epp.o: Write control 0x%x\n", val));
      // set A0 pin HIGH
      outb(inb(CTRL) | PARPORT_CONTROL_INIT, CTRL);
      outb(val, CTRL);
    }
  } else {
    DEB3(printk(KERN_DEBUG "i2c-pcf-epp.o: Write data 0x%x\n", val));
    // set A0 pin LO
    outb(inb(CTRL) & ~PARPORT_CONTROL_INIT, CTRL);
    // DEB3(printk(KERN_DEBUG "i2c-pcf-epp.o: CTRL port = 0x%x\n", inb(CTRL)));
    // DEB3(printk(KERN_DEBUG "i2c-pcf-epp.o: STAT port = 0x%x\n", inb(STAT)));
    outb(val, EDAT);
  }
}

static int pcf_epp_getbyte(void *data, int ctl)
{
  int val;

  if (ctl) {
    // set A0 pin HIGH
    outb(inb(CTRL) | PARPORT_CONTROL_INIT, CTRL);
    val = inb(EDAT);
    DEB3(printk(KERN_DEBUG "i2c-pcf-epp.o: Read control 0x%x\n", val));
  } else {
    // set A0 pin LOW
    outb(inb(CTRL) & ~PARPORT_CONTROL_INIT, CTRL);
    val = inb(EDAT);
    DEB3(printk(KERN_DEBUG "i2c-pcf-epp.o: Read data 0x%x\n", val));
  }
  return (val);
}

static int pcf_epp_getown(void *data)
{
  return (gpe.pe_own);
}


static int pcf_epp_getclock(void *data)
{
  return (gpe.pe_clock);
}

#if 0
static void pcf_epp_sleep(unsigned long timeout)
{
  schedule_timeout( timeout * HZ);
}
#endif

static void pcf_epp_waitforpin(void) {
  int timeout = 10;

  if (gpe.pe_irq > 0) {
    spin_lock_irq(&irq_driver_lock);
    if (pcf_pending == 0) {
      interruptible_sleep_on_timeout(&pcf_wait, timeout*HZ);
      //udelay(100);
    } else {
      pcf_pending = 0;
    }
    spin_unlock_irq(&irq_driver_lock);
  } else {
    udelay(100);
  }
}

static void pcf_epp_handler(int this_irq, void *dev_id, struct pt_regs *regs) {
  pcf_pending = 1;
  wake_up_interruptible(&pcf_wait);
  DEB3(printk(KERN_DEBUG "i2c-pcf-epp.o: in interrupt handler.\n"));
}


static int pcf_epp_init(void *data)
{
  if (check_region(gpe.pe_base, 5) < 0 ) {
    
    printk(KERN_WARNING "Could not request port region with base 0x%x\n", gpe.pe_base);
    return -ENODEV;
  } else {
    request_region(gpe.pe_base, 5, "i2c (EPP parallel port adapter)");
  }

  DEB3(printk(KERN_DEBUG "i2c-pcf-epp.o: init status port = 0x%x\n", inb(0x379)));
  
  if (gpe.pe_irq > 0) {
    if (request_irq(gpe.pe_irq, pcf_epp_handler, 0, "PCF8584", 0) < 0) {
      printk(KERN_NOTICE "i2c-pcf-epp.o: Request irq%d failed\n", gpe.pe_irq);
      gpe.pe_irq = 0;
    } else
      disable_irq(gpe.pe_irq);
      enable_irq(gpe.pe_irq);
  }
  // EPP mode initialize
  // enable interrupt from nINTR pin
  outb(inb(CTRL)|0x14, CTRL);
  // clear ERROR bit of STAT
  outb(inb(STAT)|0x01, STAT);
  outb(inb(STAT)&~0x01,STAT);
  
  return 0;
}

/* ------------------------------------------------------------------------
 * Encapsulate the above functions in the correct operations structure.
 * This is only done when more than one hardware adapter is supported.
 */
static struct i2c_algo_pcf_data pcf_epp_data = {
	.setpcf	    = pcf_epp_setbyte,
	.getpcf	    = pcf_epp_getbyte,
	.getown	    = pcf_epp_getown,
	.getclock   = pcf_epp_getclock,
	.waitforpin = pcf_epp_waitforpin,
	.udelay	    = 80,
	.mdelay	    = 80,
	.timeout    = HZ,
};

static struct i2c_adapter pcf_epp_ops = {
	.owner		= THIS_MODULE,
	.name		= "PCF8584 EPP adapter",
	.id		= I2C_HW_P_LP,
	.algo_data	= &pcf_epp_data,
};

static int __init i2c_pcfepp_init(void) 
{
  struct i2c_pcf_epp *pepp = &gpe;

  printk(KERN_DEBUG "i2c-pcf-epp.o: i2c pcf8584-epp adapter module version %s (%s)\n", I2C_VERSION, I2C_DATE);
  if (base == 0)
    pepp->pe_base = DEFAULT_BASE;
  else
    pepp->pe_base = base;

  if (irq == 0)
    pepp->pe_irq = DEFAULT_IRQ;
  else if (irq<0) {
    // switch off irq
    pepp->pe_irq=0;
  } else {
    pepp->pe_irq = irq;
  }
  if (clock == 0)
    pepp->pe_clock = DEFAULT_CLOCK;
  else
    pepp->pe_clock = clock;

  if (own == 0)
    pepp->pe_own = DEFAULT_OWN;
  else
    pepp->pe_own = own;

  pcf_epp_data.data = (void *)pepp;
  init_waitqueue_head(&pcf_wait);
  if (pcf_epp_init(pepp) == 0) {
    int ret;
    if ( (ret = i2c_pcf_add_bus(&pcf_epp_ops)) < 0) {
      printk(KERN_WARNING "i2c_pcf_add_bus caused an error: %d\n",ret);
      release_region(pepp->pe_base , 5);
      return ret;
    }
  } else {
    
    return -ENODEV;
  }
  printk(KERN_DEBUG "i2c-pcf-epp.o: found device at %#x.\n", pepp->pe_base);
  return 0;
}

static void __exit pcf_epp_exit(void)
{
  i2c_pcf_del_bus(&pcf_epp_ops);
  if (gpe.pe_irq > 0) {
    disable_irq(gpe.pe_irq);
    free_irq(gpe.pe_irq, 0);
  }
  release_region(gpe.pe_base , 5);
}

MODULE_AUTHOR("Hans Berglund <hb@spacetec.no> \n modified by Ryosuke Tajima <rosk@jsk.t.u-tokyo.ac.jp>");
MODULE_DESCRIPTION("I2C-Bus adapter routines for PCF8584 EPP parallel port adapter");
MODULE_LICENSE("GPL");

MODULE_PARM(base, "i");
MODULE_PARM(irq, "i");
MODULE_PARM(clock, "i");
MODULE_PARM(own, "i");
MODULE_PARM(i2c_debug, "i");

module_init(i2c_pcfepp_init);
module_exit(pcf_epp_exit);
