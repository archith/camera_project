/*
 * linux/include/asm-arm/arch-pl1029/time.h
 * 2001 Mindspeed
 */

#ifndef __ASM_ARCH_TIME_H__
#define __ASM_ARCH_TIME_H__

#include <linux/config.h>
#include <asm/irq.h>
#include <asm/arch/param.h>

#define LOG_2_HZ    7

static void timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    writeb(0, PL_TIMER_STI_CLR);    /* clear timer interrupt */
    do_timer(regs);
    do_set_rtc();
}

static void pl_time_init(void)
{
     /* setup PIT */
    writeb(LOG_2_HZ - 4, PL_TIMER_STI_P);
    // writeb(0, PL_TIMER_STI_CLR);
    writeb(1, PL_TIMER_STI_ENA);
}


/* The function is invoked by arch/arm/kernel/time.c: time_init() */
static inline void setup_timer(void)
{
    /* hook set_rtc fro setting the RTC's idea of the current time */
    /* set_rtc is invoked by arm/kernel/time.c:do_set_rtc() every ~11 minutes */
    // set_rtc = pl_rtc_doset() ...   Hi David, please modify the code for rtc adjusting
    timer_irq.handler = timer_interrupt;
    setup_arm_irq(IRQ_PL_TIMER, &timer_irq);

    pl_time_init();
}

#endif /* __ASM_ARCH_TIME_H__ */
