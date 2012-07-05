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
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <linux/i2c.h>
#include "i2c-algo-61.h"

#define VERSION     "1.2.1"

struct i2c_adapt_61 {
    int     base;
    int     irq;
    int     clock;
    int     own;
    wait_queue_head_t   i2c_wait;
    atomic_t            irq_done;
    struct timer_list   wait_timeout;
    int     status;
};

#define DEFAULT_BASE  0xd9440000
#define DEFAULT_IRQ   17
#define DEFAULT_CLOCK (100*1000)	/* default 16MHz/(27+14) = 400KHz */
#define DEFAULT_OWN   0x5C



/* ----- global defines -----------------------------------------------	*/
#define DEB(x)	if (i2c_debug>=1) x
#define DEB2(x) if (i2c_debug>=2) x
#define DEB3(x) if (i2c_debug>=3) x
#define DEBE(x)	x	/* error messages 				*/


/* ----- local functions ----------------------------------------------	*/
static void i2c_writew(void *adap_data, int reg, int val)
{
    struct i2c_adapt_61 *i2c_adapt_data = (struct i2c_adapt_61 *)adap_data;
    writew(val, i2c_adapt_data->base+reg);
}

static void i2c_writel(void *adap_data, int reg, int val)
{
    struct i2c_adapt_61 *i2c_adapt_data = (struct i2c_adapt_61 *)adap_data;
    writel(val, i2c_adapt_data->base+reg);
}

static int i2c_readw(void *adap_data, int reg)
{
    struct i2c_adapt_61 *i2c_adapt_data = (struct i2c_adapt_61 *)adap_data;
    return readw(i2c_adapt_data->base + reg);
}

static int i2c_readl(void *adap_data, int reg)
{
    struct i2c_adapt_61 *i2c_adapt_data = (struct i2c_adapt_61 *)adap_data;
    return readl(i2c_adapt_data->base + reg);
}



/* Return our slave address.  This is the address
 * put on the I2C bus when another master on the bus wants to address us
 * as a slave
 */
static int i2c_getown(void *adap_data)
{
    struct i2c_adapt_61 *i2c_adapt_data = (struct i2c_adapt_61 *)adap_data;
    return i2c_adapt_data->own;
}


static int i2c_getclock(void *adap_data)
{
    struct i2c_adapt_61 *i2c_adapt_data = (struct i2c_adapt_61 *)adap_data;
    return i2c_adapt_data->clock;
}

/* Put this process to sleep.  We will wake up when the
 * IIC controller interrupts.
 */
static void i2c_wait_for_pin(void *adap_data, int *status, int timeout)
{
    struct i2c_adapt_61 *i2c_adapt_data = (struct i2c_adapt_61 *)adap_data;

    mod_timer(&i2c_adapt_data->wait_timeout, jiffies + timeout);
    wait_event((i2c_adapt_data->i2c_wait), atomic_read(&i2c_adapt_data->irq_done) > 0);
    del_timer(&i2c_adapt_data->wait_timeout);
    atomic_dec(&i2c_adapt_data->irq_done);
    *status = i2c_adapt_data->status;
    i2c_adapt_data->status = 0;
}

#define PL_MCR_STATUS_ERR   ((0x5) << 8)

static void i2c_timeout_handler(unsigned long dev)
{
    struct i2c_adapt_61 *i2c_adapt_data = (struct i2c_adapt_61 *)dev;
    if (atomic_read(&i2c_adapt_data->irq_done) == 0) {
        i2c_adapt_data->status = PL_MCR_STATUS_ERR;
        atomic_inc(&i2c_adapt_data->irq_done);
        wake_up(&i2c_adapt_data->i2c_wait);
    }
}

static void i2c_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    struct i2c_adapt_61 *i2c_adapt_data = (struct i2c_adapt_61 *)dev_id;
    if (atomic_read(&i2c_adapt_data->irq_done) == 0) {
        i2c_adapt_data->status = readw(i2c_adapt_data->base + I2C_MCR);
        atomic_inc(&i2c_adapt_data->irq_done);
        wake_up(&i2c_adapt_data->i2c_wait);
    }
}

static int i2c_61_reg(struct i2c_client *client)
{
	return 0;
}

static int i2c_61_unreg(struct i2c_client *client)
{
	return 0;
}


/* ------------------------------------------------------------------------
 * Encapsulate the above functions in the correct operations structure.
 * This is only done when more than one hardware adapter is supported.
 */
static struct i2c_algo_61_data i2c_algo_data = {
    .data       = NULL,
    .writew     = i2c_writew,
    .writel     = i2c_writel,
    .readw      = i2c_readw,
    .readl      = i2c_readl,
	.getown     = i2c_getown,
	.getclock   = i2c_getclock,
    .wait_for_pin   = i2c_wait_for_pin,
    .timeout    = (HZ/8),
};

static struct i2c_adapter i2c_61_ops = {
    .owner          = THIS_MODULE,
    .name           = "prolific-i2c-adapter",
    .id             = I2C_HW_61,
    .algo           = NULL,
    .algo_data      = &i2c_algo_data,
    .client_register    = i2c_61_reg,
    .client_unregister  = i2c_61_unreg,
};


static struct i2c_adapt_61 i2c_adapt_data;
/* Called when the module is loaded.  This function starts the
 * cascade of calls up through the hierarchy of i2c modules (i.e. up to the
 *  algorithm layer and into to the core layer)
 */
static int i2c_clk_setup __initdata = 0;
static int i2c_cd __initdata = 0;
static int i2c_kdv __initdata = 0;
int __init i2c_61_init(void)
{
    int rc = 0;
    int i2c_clk = 0;
    int gcr_kdv;
    unsigned long gcr;
	printk(KERN_INFO "Initialize Prolific I2C adapter module v%s\n", VERSION);

    i2c_adapt_data.base = DEFAULT_BASE;
    i2c_adapt_data.irq = DEFAULT_IRQ;
    i2c_adapt_data.clock = DEFAULT_CLOCK;
    i2c_adapt_data.own = DEFAULT_OWN;
	i2c_algo_data.data = (void *)&i2c_adapt_data;
	init_waitqueue_head(&i2c_adapt_data.i2c_wait);
    atomic_set(&i2c_adapt_data.irq_done, 0);


    if (i2c_clk_setup) {
        i2c_cd = i2c_cd & 0x7;
        writeb(i2c_cd, PL_CLK_I2C);
        i2c_clk = pl_get_dev_hz() / 8;  /* jedy should be modified */
        gcr_kdv = i2c_kdv;
    } else if (pl_get_dev_hz() == 96000000) {   /* dclk = 96MHz */
        writeb(6, PL_CLK_I2C);                  /* program i2c dev clk to 24MHz */
        gcr_kdv = 10;            /* program i2c data rate = i2c_dev_clk/(20*(10+2)) */
        i2c_clk = 96000000/4;
    } else if (pl_get_dev_hz() == 32000000) {    /* dclk = 32MHz */
        writeb(5, PL_CLK_I2C);                   /* target 16MHz */
        gcr_kdv = 6;
        i2c_clk = 32000000/2;
    } else if (pl_get_dev_hz() == 120000000) {  /* dclk = 120MHz */
        writeb(6, PL_CLK_I2C);                  /* program i2c dev clk to 30MHz */
        gcr_kdv = 13;
        i2c_clk = 120000000/4;
    } else {
        writeb(7, PL_CLK_I2C);
        gcr_kdv = 32;
        i2c_clk = pl_get_dev_hz()/8;
    }

    i2c_adapt_data.clock = i2c_clk / (20 * (gcr_kdv + 2));

#if 0
    gcr = PL_GCR_I2CEN | PL_GCR_MCR_IEN | PL_GCR_SCR_IEN | PL_GCR_KDV(gcr_kdv) |
            PL_GCR_XSCL_PU | PL_GCR_XSCL_6MA |  PL_GCR_XSDA_PU | PL_GCR_XSDA_6MA;
#endif
    gcr = PL_GCR_I2CEN | PL_GCR_MCR_IEN | PL_GCR_KDV(gcr_kdv);
    i2c_writel(&i2c_adapt_data, I2C_GCR, gcr);

    rc = request_irq(i2c_adapt_data.irq, i2c_handler, 0, "I2C ADAPT", &i2c_adapt_data);
    if (rc < 0) {
        printk("Failed to enable i2c irq %d\n", i2c_adapt_data.irq);
        rc = -ENODEV;
        goto EXIT;
    }
    /* enable_irq(i2c_adapt_data.irq); */  // it's redundant

    init_timer(&i2c_adapt_data.wait_timeout);
    i2c_adapt_data.wait_timeout.function = i2c_timeout_handler;
    i2c_adapt_data.wait_timeout.data = (unsigned long) &i2c_adapt_data;



    if (i2c_61_add_bus(&i2c_61_ops) < 0) {
        rc = -ENODEV;
        goto EXIT;
    }
	printk(KERN_INFO " found i2c adapter at %#x irq %d. Data tranfer clock is %dHz\n",
		i2c_adapt_data.base, i2c_adapt_data.irq, i2c_adapt_data.clock);


EXIT:
    return rc;
}


static void i2c_61_exit(void)
{
	i2c_61_del_bus(&i2c_61_ops);
    disable_irq(i2c_adapt_data.irq);
    free_irq(i2c_adapt_data.irq, &i2c_adapt_data);
}


/*
 * Syntax:
 *  i2c=<i2c_cd>,<i2c_kdv>
 *  <i2c_cd> is 0 ~ 7
 *  <i2c_kdv> is 0 ~ 63
 */
static int __init i2c_setup(char *options)
{
    i2c_cd = simple_strtoul(options, &options, 0);
    i2c_cd = i2c_cd & 0x7;
    if (*options != ',')
        i2c_kdv = 32;
    else {
        options++;
        i2c_kdv = simple_strtoul(options, &options, 0);
    }

    i2c_clk_setup = 1;

    return 1;
}

__setup("i2c=", i2c_setup);

EXPORT_NO_SYMBOLS;

/* If modules is NOT defined when this file is compiled, then the MODULE_*
 * macros will resolve to nothing
 */
MODULE_AUTHOR("Jedy H.P. Wei <jedy-wei@prolific.com.tw>");
MODULE_DESCRIPTION("I2C-Bus adapter routines PL1029 i2c host");
MODULE_LICENSE("GPL");

MODULE_PARM(i2c_debug,"i");


/* Called when module is loaded or when kernel is intialized.
 * If MODULES is defined when this file is compiled, then this function will
 * resolve to init_module (the function called when insmod is invoked for a
 * module).  Otherwise, this function is called early in the boot, when the
 * kernel is intialized.  Check out /include/init.h to see how this works.
 */
module_init(i2c_61_init);

/* Resolves to module_cleanup when MODULES is defined. */
module_exit(i2c_61_exit);
