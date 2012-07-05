/*
   -------------------------------------------------------------------------
   i2c-adap-ibm_ocp.c i2c-hw access for the IIC peripheral on the IBM PPC 405
   -------------------------------------------------------------------------
  
   Ian DaSilva, MontaVista Software, Inc.
   idasilva@mvista.com or source@mvista.com

   Copyright 2000 MontaVista Software Inc.

   Changes made to support the IIC peripheral on the IBM PPC 405 


   ----------------------------------------------------------------------------
   This file was highly leveraged from i2c-elektor.c, which was created
   by Simon G. Vogl and Hans Berglund:

 
     Copyright (C) 1995-97 Simon G. Vogl
                   1998-99 Hans Berglund

   With some changes from Kyösti Mälkki <kmalkki@cc.hut.fi> and even
   Frodo Looijaard <frodol@dds.nl>


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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
   ----------------------------------------------------------------------------

   History: 01/20/12 - Armin
   	akuster@mvista.com
   	ported up to 2.4.16+	

   Version 02/03/25 - Armin
       converted to ocp format
       removed commented out or #if 0 code

   TODO: convert to ocp_register
         add PM hooks

*/


#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-ibm_ocp.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/ocp.h>

/*
 * This next section is configurable, and it is used to set the number
 * of i2c controllers in the system.  The default number of instances is 1,
 * however, this should be changed to reflect your system's configuration.
 */ 

/*
 * The STB03xxx, with a PPC405 core, has two i2c controllers.
 */
//(sizeof(IIC_ADDR)/sizeof(struct iic_regs))
extern iic_t *IIC_ADDR[];
static struct iic_ibm iic_ibmocp_adaps[IIC_NUMS][5];

static struct i2c_algo_iic_data *iic_ibmocp_data[IIC_NUMS];
static struct i2c_adapter *iic_ibmocp_ops[IIC_NUMS];

static int i2c_debug=0;
static wait_queue_head_t iic_wait[IIC_NUMS];
static int iic_pending;
static spinlock_t irq_driver_lock = SPIN_LOCK_UNLOCKED;


/* ----- global defines -----------------------------------------------	*/
#define DEB(x)	if (i2c_debug>=1) x
#define DEB2(x) if (i2c_debug>=2) x
#define DEB3(x) if (i2c_debug>=3) x
#define DEBE(x)	x	/* error messages 				*/

/* ----- local functions ----------------------------------------------	*/

//
// Description: Write a byte to IIC hardware
//
static void iic_ibmocp_setbyte(void *data, int ctl, int val)
{
   // writeb resolves to a write to the specified memory location
   // plus a call to eieio.  eieio ensures that all instructions
   // preceding it are completed before any further stores are
   // completed.
   // Delays at this level (to protect writes) are not needed here.
   writeb(val, ctl);
}


//
// Description: Read a byte from IIC hardware
//
static int iic_ibmocp_getbyte(void *data, int ctl)
{
   int val;

   val = readb(ctl);
   return (val);
}


//
// Description: Return our slave address.  This is the address
// put on the I2C bus when another master on the bus wants to address us
// as a slave
//
static int iic_ibmocp_getown(void *data)
{
   return(((struct iic_ibm *)(data))->iic_own);
}


//
// Description: Return the clock rate
//
static int iic_ibmocp_getclock(void *data)
{
   return(((struct iic_ibm *)(data))->iic_clock);
}



//
// Description:  Put this process to sleep.  We will wake up when the
// IIC controller interrupts.
//
static void iic_ibmocp_waitforpin(void *data) {

   int timeout = 2;
   struct iic_ibm *priv_data = data;

   //
   // If interrupts are enabled (which they are), then put the process to
   // sleep.  This process will be awakened by two events -- either the
   // the IIC peripheral interrupts or the timeout expires. 
   //
   if (priv_data->iic_irq > 0) {
      spin_lock_irq(&irq_driver_lock);
      if (iic_pending == 0) {
  	 interruptible_sleep_on_timeout(&(iic_wait[priv_data->index]), timeout*HZ );
      } else
 	 iic_pending = 0;
      spin_unlock_irq(&irq_driver_lock);
   } else {
      //
      // If interrupts are not enabled then delay for a reasonable amount
      // of time and return.  We expect that by time we return to the calling
      // function that the IIC has finished our requested transaction and
      // the status bit reflects this.
      //
      // udelay is probably not the best choice for this since it is
      // the equivalent of a busy wait
      //
      udelay(100);
   }
   //printk("iic_ibmocp_waitforpin: exitting\n");
}


//
// Description: The registered interrupt handler
//
static void iic_ibmocp_handler(int this_irq, void *dev_id, struct pt_regs *regs) 
{
   int ret;
   struct iic_regs *iic;
   struct iic_ibm *priv_data = dev_id;
   iic = (struct iic_regs *) priv_data->iic_base;
   iic_pending = 1;
   DEB2(printk("iic_ibmocp_handler: in interrupt handler\n"));
   // Read status register
   ret = readb((int) &(iic->sts));
   DEB2(printk("iic_ibmocp_handler: status = %x\n", ret));
   // Clear status register.  See IBM PPC 405 reference manual for details
   writeb(0x0a, (int) &(iic->sts));
   wake_up_interruptible(&(iic_wait[priv_data->index]));
}


//
// Description: This function is very hardware dependent.  First, we lock
// the region of memory where out registers exist.  Next, we request our
// interrupt line and register its associated handler.  Our IIC peripheral
// uses interrupt number 2, as specified by the 405 reference manual.
//
static int iic_hw_resrc_init(int instance)
{

   DEB(printk("iic_hw_resrc_init: Physical Base address: 0x%x\n", (u32) IIC_ADDR[instance] ));
   iic_ibmocp_adaps[instance]->iic_base = (u32)ioremap((unsigned long)IIC_ADDR[instance],PAGE_SIZE);

   DEB(printk("iic_hw_resrc_init: ioremapped base address: 0x%x\n", iic_ibmocp_adaps[instance]->iic_base));

   if (iic_ibmocp_adaps[instance]->iic_irq > 0) {
	
      if (request_irq(iic_ibmocp_adaps[instance]->iic_irq, iic_ibmocp_handler,
       0, "IBM OCP IIC", iic_ibmocp_adaps[instance]) < 0) {
         printk(KERN_ERR "iic_hw_resrc_init: Request irq%d failed\n",
          iic_ibmocp_adaps[instance]->iic_irq);
	 iic_ibmocp_adaps[instance]->iic_irq = 0;
      } else {
         DEB3(printk("iic_hw_resrc_init: Enabled interrupt\n"));
      }
   }
   return 0;
}


//
// Description: Release irq and memory
//
static void iic_ibmocp_release(void)
{
   int i;

   for(i=0; i<IIC_NUMS; i++) {
      struct iic_ibm *priv_data = (struct iic_ibm *)iic_ibmocp_data[i]->data;
      if (priv_data->iic_irq > 0) {
         disable_irq(priv_data->iic_irq);
         free_irq(priv_data->iic_irq, 0);
      }
      kfree(iic_ibmocp_data[i]);
      kfree(iic_ibmocp_ops[i]);
   }
}


//
// Description: Called when the module is loaded.  This function starts the
// cascade of calls up through the heirarchy of i2c modules (i.e. up to the
//  algorithm layer and into to the core layer)
//
static int __init iic_ibmocp_init(void) 
{
   int i;

   printk(KERN_INFO "iic_ibmocp_init: IBM on-chip iic adapter module\n");
 
   for(i=0; i<IIC_NUMS; i++) {
      iic_ibmocp_data[i] = kmalloc(sizeof(struct i2c_algo_iic_data),GFP_KERNEL);
      if(iic_ibmocp_data[i] == NULL) {
         return -ENOMEM;
      }
      memset(iic_ibmocp_data[i], 0, sizeof(struct i2c_algo_iic_data));
      
      switch (i) {
	      case 0:
	       iic_ibmocp_adaps[i]->iic_irq = IIC_IRQ(0);
	      break;
	      case 1:
	       iic_ibmocp_adaps[i]->iic_irq = IIC_IRQ(1);
	      break;
      }
      iic_ibmocp_adaps[i]->iic_clock = IIC_CLOCK;
      iic_ibmocp_adaps[i]->iic_own = IIC_OWN; 
      iic_ibmocp_adaps[i]->index = i;
 
      DEB(printk("irq %x\n", iic_ibmocp_adaps[i]->iic_irq));
      DEB(printk("clock %x\n", iic_ibmocp_adaps[i]->iic_clock));
      DEB(printk("own %x\n", iic_ibmocp_adaps[i]->iic_own));
      DEB(printk("index %x\n", iic_ibmocp_adaps[i]->index));


      iic_ibmocp_data[i]->data = (struct iic_regs *)iic_ibmocp_adaps[i]; 
      iic_ibmocp_data[i]->setiic = iic_ibmocp_setbyte;
      iic_ibmocp_data[i]->getiic = iic_ibmocp_getbyte;
      iic_ibmocp_data[i]->getown = iic_ibmocp_getown;
      iic_ibmocp_data[i]->getclock = iic_ibmocp_getclock;
      iic_ibmocp_data[i]->waitforpin = iic_ibmocp_waitforpin;
      iic_ibmocp_data[i]->udelay = 80;
      iic_ibmocp_data[i]->mdelay = 80;
      iic_ibmocp_data[i]->timeout = HZ;
      
            iic_ibmocp_ops[i] = kmalloc(sizeof(struct i2c_adapter), GFP_KERNEL);
      if(iic_ibmocp_ops[i] == NULL) {
         return -ENOMEM;
      }
      memset(iic_ibmocp_ops[i], 0, sizeof(struct i2c_adapter));
      strcpy(iic_ibmocp_ops[i]->name, "IBM OCP IIC adapter");
      iic_ibmocp_ops[i]->owner = THIS_MODULE;
      iic_ibmocp_ops[i]->id = I2C_HW_OCP;
      iic_ibmocp_ops[i]->algo = NULL;
      iic_ibmocp_ops[i]->algo_data = iic_ibmocp_data[i];
       
      
      init_waitqueue_head(&(iic_wait[i]));
      if (iic_hw_resrc_init(i) == 0) {
         if (i2c_ocp_add_bus(iic_ibmocp_ops[i]) < 0)
         return -ENODEV;
      } else {
         return -ENODEV;
      }
      DEB(printk(KERN_INFO "iic_ibmocp_init: found device at %#x.\n\n", iic_ibmocp_adaps[i]->iic_base));
   }
   return 0;
}


static void __exit iic_ibmocp_exit(void)
{
   int i;

   for(i=0; i<IIC_NUMS; i++) {
      i2c_ocp_del_bus(iic_ibmocp_ops[i]);
   }
   iic_ibmocp_release();
}

//
// If modules is NOT defined when this file is compiled, then the MODULE_*
// macros will resolve to nothing
//
MODULE_AUTHOR("MontaVista Software <www.mvista.com>");
MODULE_DESCRIPTION("I2C-Bus adapter routines for PPC 405 IIC bus adapter");
MODULE_LICENSE("GPL");

MODULE_PARM(base, "i");
MODULE_PARM(irq, "i");
MODULE_PARM(clock, "i");
MODULE_PARM(own, "i");
MODULE_PARM(i2c_debug,"i");


module_init(iic_ibmocp_init);
module_exit(iic_ibmocp_exit); 
