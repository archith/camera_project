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
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/signal.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/system.h>

#include <asm/pl1061/pl1061.h>      // 1061 register definition.
#include <asm/pl1061/falarm.h>


#define VERSION_INFO  "PL-1061B Fine Alarm Driver version 0.10 (2003-09-19)\n" 
#define FALARM_MAJOR    151

#define PINFO(fmt...)

static void inline mask_falarm_irq(void)
{
    writeb(readb(rFALM_INTDIS)|1, rFALM_INTDIS);
}

static void inline unmask_falarm_irq(void)
{
    writeb(readb(rFALM_INTDIS)&0xFE, rFALM_INTDIS);
}


static devfs_handle_t   falarm_dev;
static int              dev_is_free=1;
static struct semaphore open_sem;


struct falarm_info {
    short   set;        // Alarm is set or not.
    short   pause;      // Alarm is paused now.
    int     pid;        // The PID of who sets the alarm.
    struct falarm_alarm     alarm;
    struct falarm_time      last_set;
};


static struct falarm_info   info;


extern int rtc_request_irq(unsigned int irq,
        void (*handler)(int, void *, struct pt_regs *),
        unsigned long irqflags, const char *devname, void *dev_id);
extern void rtc_free_irq(unsigned int irq, void *dev_id);


static void reset_alarm(struct falarm_alarm *alm, int init)
{
    unsigned int    ts_hi1, ts_lo;  // For getting 64-bit timestamp.
    unsigned int    ts_hi2;
    unsigned int    tm_lo=0, tm_hi;

    if (init) {
        // Alarm creation, use init_value.
        PINFO("[falarm] alarm sec [%u], usec [%u]\n", 
                alm->init_value.sec, alm->init_value.usec);
        tm_hi = alm->init_value.sec;
        tm_lo = alm->init_value.usec>>16;

        // Get current timestamp.
        ts_hi1 = readl(rFTIMESTAMP+4);
        ts_lo = readl(rFTIMESTAMP)>>16;

        ts_hi2 = readl(rFTIMESTAMP+4);

        if (ts_hi1 != ts_hi2)
            ts_lo = readl(rFTIMESTAMP)>>16;
    }
    else {
        // Alarm reset, use next_value.
        PINFO("[falarm] alarm sec [%d], usec [%u]\n", 
                alm->next_value.sec, alm->next_value.usec);
        tm_hi = alm->next_value.sec;
        tm_lo = alm->next_value.usec>>16;

        // Use info.last_set as reference.
        ts_hi2 = info.last_set.sec;
        ts_lo = info.last_set.usec>>16;
    }

    PINFO("[falarm] ts_hi [0x%x] ts_lo [0x%x]\n", ts_hi2, ts_lo);
    PINFO("[falarm] tm_hi [0x%x] tm_lo [0x%x]\n", tm_hi, tm_lo);

    tm_lo += ts_lo;

    tm_hi += (tm_lo >= 0x10000);
    tm_lo = (tm_lo & 0xFFFF) << 16;

    tm_hi += ts_hi2;

    PINFO("[falarm] setting tm_hi [0x%x] tm_lo [0x%x]\n", tm_hi, tm_lo);

    // Save this value as the next reference.
    info.last_set.sec = tm_hi;
    info.last_set.usec = tm_lo;


    // Update the fine alarm.
    writel(tm_lo, rALM_FINE);
    writel(tm_hi, rALM_FINE+4);
}


static void falarm_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    // Clear the interrupt status.
    writeb(0xFF, rFALM_INTCLR);

    if (!info.set || info.pause)
        return;

    // Send SIGALRM to the process.
    kill_proc (info.pid, SIGALRM, 1);

    // Restart the alarm.
    reset_alarm(&info.alarm, 0);
}



static int falarm_open(struct inode *inode, struct file *filp)
{
    int     ret;


    PINFO("%s\n", __FUNCTION__);

    down_interruptible(&open_sem);
    if (!dev_is_free) {
        up(&open_sem);
        return -EBUSY;
    }

    dev_is_free = 0;
    up(&open_sem);

    memset(&info, 0, sizeof(info));


    // Request the IRQ.
extern int rtc_request_irq(unsigned int irq,
        void (*handler)(int, void *, struct pt_regs *),
        unsigned long irqflags, const char *devname, void *dev_id);

    if ((ret=rtc_request_irq(1, falarm_interrupt, SA_INTERRUPT, 
                        "falarm", NULL)) != 0)  {
        printk(KERN_ERR "[falarm] request_irq failed: %d\n", ret);
        return ret;
    }

    return 0;
}


static int falarm_release(struct inode *inode, struct file *filp)
{
    PINFO("%s\n", __FUNCTION__);


    down_interruptible(&open_sem);
    if (dev_is_free) {
        up(&open_sem);
        return -EBADF;      // The device is not opened correctly.
    }

    rtc_free_irq(1, NULL);  // Release the IRQ handling.

    // Clear interrup vector.
    writeb(0xFF, rFALM_INTCLR);

    dev_is_free = 1;
    up(&open_sem);

    return 0;
}


static int falarm_ioctl(struct inode *inode, struct file *filp,
    unsigned int cmd, unsigned long arg)
{
    struct falarm_alarm *pa;


    PINFO("%s\n", __FUNCTION__);

    switch (cmd) {
        case FALARM_CLEAR:
            mask_falarm_irq();
            memset(&info, 0, sizeof(info));
            break;

        case FALARM_PAUSE:
            mask_falarm_irq();
            info.set = 0;
            break;

        case FALARM_RESUME:
            info.set = 1;
            reset_alarm(&info.alarm, 0);
            unmask_falarm_irq();
            break;

        case FALARM_SET:
            if (!access_ok(VERIFY_READ, arg, sizeof(struct falarm_alarm)))
                return -EFAULT;


            pa = (struct falarm_alarm *)arg;

            copy_from_user(&info.alarm, (void *)arg, 
                           sizeof(struct falarm_alarm));


            info.set = 1;
            info.pause = 0;
            info.pid = current->pid;

            // Set the fine alarm.
            reset_alarm(&info.alarm, 1);

            // Enable the alarm.
            unmask_falarm_irq();

            break;
    }

    return 0;
}


static struct file_operations falarm_fops =
{
    owner:      THIS_MODULE,
    open:       falarm_open,
    release:    falarm_release,
    ioctl:      falarm_ioctl,
};



int __init falarm_init(void)
{
    PINFO(VERSION_INFO);

    //---------------------------------------------
    // Disable the fine alarm first.
    // Do this operation before setting the alarm.
    //---------------------------------------------
    //mask_falarm_irq();

    sema_init(&open_sem, 1);


    // Register devfs.
    if (devfs_register_chrdev(FALARM_MAJOR, "falarm", &falarm_fops))
        return -ENXIO;

    falarm_dev=devfs_register(NULL, "falarm", DEVFS_FL_DEFAULT,
                                FALARM_MAJOR, 0, S_IFCHR|S_IRUGO|S_IWUGO,
                                &falarm_fops, (void *)NULL);
    if (falarm_dev == NULL) {
        printk("[falarm] devfs_register failed\n");
        return -ENXIO;
    }


    return 0;
}


void __exit falarm_cleanup(void)
{
    devfs_unregister(falarm_dev);
}

/*
module_init(falarm_init);
module_exit(falarm_cleanup);
*/


