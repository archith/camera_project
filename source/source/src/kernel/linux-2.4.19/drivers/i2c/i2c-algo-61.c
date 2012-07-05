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
/* ----------------------------------------------------------------------------
   This file was highly leveraged from i2c-algo-pcf.c, which was created
   by Simon G. Vogl and Hans Berglund:

     Copyright (C) 1995-1997 Simon G. Vogl
                   1998-2000 Hans Berglund

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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <linux/sched.h>

#include <linux/i2c.h>
#include "i2c-algo-61.h"

/* ----- global defines ----------------------------------------------- */
#define DEB(x) if (i2c_debug>=1) x
#define DEB2(x) if (i2c_debug>=2) x
#define DEB3(x) if (i2c_debug>=3) x /* print several statistical values*/
#define DEBPROTO(x) if (i2c_debug>=9) x;
/* debug the protocol by showing transferred bits */

/* ----- global variables ---------------------------------------------	*/

/* module parameters:
 */
static int i2c_debug=1;
// static int i2c_test=0;	/* see if the line-setting functions work	*/
// static int i2c_scan=0;	/* have a look at what's hanging 'round		*/

/* --- setting states on the bus with the right timing: ---------------	*/

#define get_clock(algo_data) algo_data->getclock(algo_data->data)
#define i2c_writew(algo_data, reg, val) algo_data->writew(algo_data->data, reg, val)
#define i2c_writel(algo_data, reg, val) algo_data->writel(algo_data->data, reg, val)
#define i2c_readw(algo_data, reg, val) algo_data->readw(algo_data->data, reg, val)
#define i2c_readl(algo_data, reg, val) algo_data->readl(algo_data->data, reg, val)

/* --- other auxiliary functions --------------------------------------	*/


/* After we issue a transaction on the i2c bus, this function
 * is called.  It puts this process to sleep until we get an interrupt from
 * from the controller telling us that the transaction we requested in complete.
 */
static void inline wait_for_pin(struct i2c_algo_61_data *algo_data, int *status)
{
    algo_data->wait_for_pin(algo_data->data, status, algo_data->timeout);
}


static void i2c_stop(struct i2c_algo_61_data *algo_data)
{
    int status;
    i2c_writel(algo_data, I2C_MCR, 0);
    wait_for_pin(algo_data, &status);   /* *** must *** */
}

#if 0
static void i2c_61_reset(struct i2c_algo_61_data *algo_data)
{
    int status;
    i2c_writel(algo_data, I2C_MCR, 0);
}
#endif


/* ----- Utility functions
 */


/* Verify the device we want to talk to on the i2c bus really exists. */
static inline int try_address(struct i2c_algo_61_data *algo_data,
            int cmd, u32 mcr_addr, int retries)
{
    int i, ret = -1;
    int status;
    unsigned long mcr;

    mcr = mcr_addr| cmd;

    for (i=0;i<retries;i++) {
        i2c_writel(algo_data, I2C_MCR, mcr);
        wait_for_pin(algo_data, &status);
        if (PL_MCR_STATUS(status) == PL_MCR_STATUS_ACK) {
            ret=0;
            break;	/* success! */
        }
        i2c_stop(algo_data);
    }
	DEB2(if (i) printk("try_address: needed %d retries for 0x%x\n",i, mcr_addr >> 16));
	return ret;
}


/* Whenever we initiate a transaction, the first byte clocked
 * onto the bus after the start condition is the address (7 bit) of the
 * device we want to talk to.  This function manipulates the address specified
 * so that it makes sense to the hardware when written to the i2c peripheral.
 *
 * Note: 10 bit addresses are not supported in this driver, although they are
 * supported by the hardware.  This functionality needs to be implemented.
 */
static inline int i2c_doAddress(struct i2c_algo_61_data *algo_data, u32 mcr_addr,
                                struct i2c_msg *msg, int retries)
{
	int ret;
    int cmd = (msg->flags & I2C_M_RD) ? PL_MCR_CMD_RD : PL_MCR_CMD_WR;


    ret = try_address(algo_data, cmd, mcr_addr, retries);
    if (ret < 0) {
	    printk(KERN_DEBUG "i2c_doAddress: died at address code.\n");
		return -EREMOTEIO;
    }

	return 0;
}


static int i2c_sendbytes(struct i2c_algo_61_data *algo_data, u32 mcr_addr, const unsigned char *buf, int count)
{
	int wrcount=0;
	int status;
	int i;
    u32 mcr;

    mcr = mcr_addr | PL_MCR_WR_DATA;

    for (i = 0; i < count; i++) {
        i2c_writew(algo_data, I2C_MCR, mcr | buf[wrcount++]);
        wait_for_pin(algo_data, &status);
        if (PL_MCR_STATUS(status) != PL_MCR_STATUS_ACK)
            break;
    }
    return wrcount;
}


static int i2c_readbytes(struct i2c_algo_61_data *algo_data, u32 mcr_addr, unsigned char *buf, int count)
{
	int rdcount=0;
	int status;
	int i;
    u32 mcr;

    mcr = mcr_addr | PL_MCR_RD_NEXT;

    for (i = 0; i < count; i++) {
        i2c_writew(algo_data, I2C_MCR, mcr);
        wait_for_pin(algo_data, &status);
        if (PL_MCR_STATUS(status) != PL_MCR_STATUS_ACK)
            break;
        buf[rdcount++] = status & PL_MCR_DATA_MASK;
    }

    return rdcount;
}


/* This function implements combined transactions.  Combined
 * transactions consist of combinations of reading and writing blocks of data.
 * Each transfer (i.e. a read or a write) is separated by a repeated start
 * condition.
 */
static int i2c_combined_transaction(struct i2c_algo_61_data *algo_data, struct i2c_msg msgs[], int num, int retries)
{
    int i;
    struct i2c_msg *pmsg;
    int ret = 0;
    int rwcount = 0;
    u32 mcr_addr;

    DEB2(printk("Beginning combined transaction\n"));

    mcr_addr = PL_MCR_ADDR((msgs[0].flags & I2C_M_TEN),msgs[0].addr);

    for(i=0; i<num; i++) {
        pmsg = &msgs[i];
        ret = i2c_doAddress(algo_data, mcr_addr, pmsg, retries);
        if (ret < 0)
            goto EXIT;
        if(pmsg->flags & I2C_M_RD) {
            DEB2(printk("  This one is a read\n"));
            ret = i2c_readbytes(algo_data, mcr_addr, pmsg->buf, pmsg->len);
            if (ret < 0)
                break;
            rwcount += ret;
        } else if(!(pmsg->flags & I2C_M_RD)) {
            DEB2(printk("This one is a write\n"));
            ret = i2c_sendbytes(algo_data, mcr_addr, pmsg->buf, pmsg->len);
            if (ret < 0)
                break;
            rwcount += ret;
        }
    }
    i2c_stop(algo_data);

EXIT:
    if (ret < 0)
        return ret;
    else
        return rwcount;
}


/* Description: Prepares the controller for a transaction (clearing status
 * registers, data buffers, etc), and then calls either i2c_readbytes or
 * i2c_sendbytes to do the actual transaction.
 *
 * still to be done: Before we issue a transaction, we should
 * verify that the bus is not busy or in some unknown state.
 */
static int i2c_xfer(struct i2c_adapter *i2c_adap,
		    struct i2c_msg msgs[],
		    int num)
{
    struct i2c_algo_61_data *algo_data = i2c_adap->algo_data;
    struct i2c_msg *pmsg;
    int i = 0;
    int ret;
    u32 mcr_addr;

    pmsg = &msgs[i];

    if (num > 1) {
        ret = i2c_combined_transaction(algo_data, msgs, num, i2c_adap->retries);
        goto EXIT;
    }

    mcr_addr = PL_MCR_ADDR((pmsg->flags & I2C_M_TEN), pmsg->addr);

    /* Load address */
    ret = i2c_doAddress(algo_data, mcr_addr, pmsg, i2c_adap->retries);
    if (ret < 0)
        goto EXIT;
    /* Just test address */
    if (pmsg->len == 0) {
        i2c_stop(algo_data);
        goto EXIT;
    }

    DEB3(printk("i2c_xfer: Msg %d, addr=0x%x, flags=0x%x, len=%d\n",	i, msgs[i].addr, msgs[i].flags, msgs[i].len);)

    if(pmsg->flags & I2C_M_RD) 		/* Read */
        ret = i2c_readbytes(algo_data, mcr_addr, pmsg->buf, pmsg->len);
    else            				/* Write */
        ret = i2c_sendbytes(algo_data, mcr_addr, pmsg->buf, pmsg->len);

    i2c_stop(algo_data);

EXIT:
    if (ret > 0)
        DEB3(printk("i2c_xfer: read/write %d bytes.\n",ret));

	return ret;
}


/* Implements device specific ioctls.  Higher level ioctls can
 * be found in i2c-core.c and are typical of any i2c controller (specifying
 * slave address, timeouts, etc).  These ioctls take advantage of any hardware
 * features built into the controller for which this algorithm-adapter set
 * was written.  These ioctls allow you to take control of the data and clock
 * lines and set the either high or low,
 * similar to a GPIO pin.
 */
static int i2c_ioctl(struct i2c_adapter *i2c_adap,
	unsigned int cmd, unsigned long arg)
{

#if 0
    struct i2c_algo_61_data *algo_data = i2c_adap->algo_data;
    struct i2c_i2c_msg s_msg;
    char *buf;
    int ret;

    if (cmd == I2C_SREAD) {
	    if(copy_from_user(&s_msg, (struct i2c_i2c_msg *)arg,
		        sizeof(struct i2c_i2c_msg)))
			return -EFAULT;
		buf = kmalloc(s_msg.len, GFP_KERNEL);
		if (buf== NULL)
			return -ENOMEM;

		/* Flush FIFO */
		i2c_outw(algo_data, ITE_I2CFCR, ITE_I2CFCR_FLUSH);

		/* Load address */
		i2c_outw(algo_data, ITE_I2CSAR,s_msg.addr<<1);
		i2c_outw(algo_data, ITE_I2CSSAR,s_msg.waddr & 0xff);

		ret = i2c_readbytes(algo_data, buf, s_msg.len);
		if (ret>=0) {
			if(copy_to_user( s_msg.buf, buf, s_msg.len) )
				ret = -EFAULT;
		}
		kfree(buf);
	}
#endif
    return 0;
}


static u32 i2c_func(struct i2c_adapter *i2c_adap)
{
	return I2C_FUNC_SMBUS_EMUL | I2C_FUNC_10BIT_ADDR ;
}

/* -----exported algorithm data: -------------------------------------	*/

static struct i2c_algorithm i2c_algo = {
    .owner  = THIS_MODULE,
	.name   = "prolific-i2c-algorithm",
	.id     = I2C_ALGO_61,
	.master_xfer    = i2c_xfer,
	.algo_control   = i2c_ioctl,
	.functionality  = i2c_func,
};


/*
 * registering functions to load algorithms at runtime
 */
int i2c_61_add_bus(struct i2c_adapter *i2c_adap)
{
	DEB2(printk("i2c-algo-61: hw routines for %s registered.\n",
	            i2c_adap->name));

	/* register new adapter to i2c module... */

	i2c_adap->id |= i2c_algo.id;
	i2c_adap->algo = &i2c_algo;

	i2c_adap->timeout = 100;	/* default values, should	*/
	i2c_adap->retries = 3;		/* be replaced by defines	*/
	i2c_adap->flags = 0;

#ifdef MODULE
	MOD_INC_USE_COUNT;
#endif

	i2c_add_adapter(i2c_adap);

#if 0
	struct i2c_algo_61_data *algo_data = i2c_adap->algo_data;


	/* scan bus */
	/* By default scanning the bus is turned off. */
	if (i2c_scan) {
		printk(KERN_INFO " i2c-algo-ite: scanning bus %s.\n",
		       algo_data->name);
		for (i = 0x00; i < 0xff; i+=2) {
			i2c_outw(i2c_adap, ITE_I2CSAR, i);
			i2c_start(i2c_adap);
			wait_for_pin(i2c_adap, &status);
            if(((status & ITE_I2CHSR_DNE) == 0) ) {
				printk(KERN_INFO "\n(%02x)\n",i>>1);
			} else {
				printk(KERN_INFO ".");
				i2c_reset(i2c_adap);
			}
		}
	}
#endif
	return 0;
}


int i2c_61_del_bus(struct i2c_adapter *i2c_adap)
{
	int res;
	res = i2c_del_adapter(i2c_adap);
#ifdef MODULE
	MOD_DEC_USE_COUNT;
#endif
	return 0;
}


int __init i2c_algo_61_init (void)
{
	printk(KERN_INFO "Prolific i2c algorithm module v1.2\n");
	return 0;
}


EXPORT_SYMBOL(i2c_61_add_bus);
EXPORT_SYMBOL(i2c_61_del_bus);

/* The MODULE_* macros resolve to nothing if MODULES is not defined
 * when this file is compiled.
 */
MODULE_AUTHOR("Jedy H.P. Wei <jedy-wei@prolific.com.tw>");
MODULE_DESCRIPTION("PL1061 i2c algorithm");
MODULE_LICENSE("GPL");


// MODULE_PARM(i2c_test, "i");
// MODULE_PARM(i2c_scan, "i");
MODULE_PARM(i2c_debug,"i");

// MODULE_PARM_DESC(i2c_test, "Test if the I2C bus is available");
// MODULE_PARM_DESC(i2c_scan, "Scan for active chips on the bus");
MODULE_PARM_DESC(i2c_debug,
        "debug level - 0 off; 1 normal; 2,3 more verbose; 9 i2c-protocol");


/* This function resolves to init_module (the function invoked when a module
 * is loaded via insmod) when this file is compiled with MODULES defined.
 * Otherwise (i.e. if you want this driver statically linked to the kernel),
 * a pointer to this function is stored in a table and called
 * during the intialization of the kernel (in do_basic_setup in /init/main.c)
 *
 * All this functionality is complements of the macros defined in linux/init.h
 */
module_init(i2c_algo_61_init);

