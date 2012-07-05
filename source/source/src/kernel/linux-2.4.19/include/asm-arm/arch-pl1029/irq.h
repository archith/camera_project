/*
 * asm/arch-atmel/irq.h:
 * 2001 Erwin Authried
 *
 */

#ifndef __ASM_ARCH_IRQ_H__
#define __ASM_ARCH_IRQ_H__

#include <asm/hardware.h>
#include <asm/io.h>

#define MAX_LEVEL2_IRQ      16
#define LEVEL3_IRQ_OFFSET   16

/* function define in asm/mach-pl1097/irq.c */
static int inline fixup_irq(int irq) {
    if (irq == IRQ_PL_DMAC0) { /* LEVEL-3 Interrupt? */
        irq = (readl(PL_DMA_INT_VECTOR) >> 2) + LEVEL3_IRQ_OFFSET;
    }
    return irq;
}

#endif /* __ASM_ARCH_IRQ_H__ */
