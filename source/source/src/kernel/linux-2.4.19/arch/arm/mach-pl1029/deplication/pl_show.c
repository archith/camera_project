#include <linux/config.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/timex.h>
#include <linux/malloc.h>
#include <linux/random.h>

#include <asm/bitops.h>
// #include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
// #include <asm/mipsregs.h>
#include <asm/system.h>
// #include <asm/sni.h>
// #include <asm/nile4.h>
#include <asm/pl_reg.h>

//#include <asm/pl_irq.h>
//Modified by chun-wen 20040709
#include <asm/arch/irqs.h>


void pl_show_status(char xx){
     unsigned char *pLED = rLED_BASE;
     *pLED = xx;
}

asmlinkage void pl_debug_IRQ(int irq, struct pt_regs *regs)
{
    struct irqaction *action;
    int do_random, cpu;
    /* clear system tick interrupt signal */
    // byteAddr(rTICKER_CLEAR) = 0;
    pl_printk("debug_irq happen !!! \n");

    (*(unsigned long *)&jiffies)++;
    // __cli();
    /* unmasking and bottom half handling is done magically for us. */
}

asmlinkage void pl_show_data(unsigned int x)
{
     pl_printk("data=%08x\n",x);
}
