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
#include <linux/slab.h>
#include <linux/random.h>

#include <asm/bitops.h>
#include <asm/io.h>

#include <asm/irq.h>
#include <asm/system.h>
#include <asm/mach/irq.h>
#include <asm/arch/irq.h>
#include <asm/hardware.h>



static void pl_mask_irq(unsigned int irq)
{
    unsigned long status;

    if (irq >= NR_IRQS)
        return;

    /* mask the irq */
    status = readw(PL_INT_MASK);

    /* Check level 2 or level 3 interrupt */
    if (irq <  MAX_LEVEL2_IRQ)
        writew((status & ~(1 << irq)), PL_INT_MASK);
    else {
    /* Mask Level 3 IRQ */
        status = readw(PL_DMA_INT_MASK);
        writew((status & ~(1 << (irq - LEVEL3_IRQ_OFFSET))), PL_DMA_INT_MASK);
    }

}

static inline void pl_unmask_irq(unsigned int irq)
{
    unsigned long status;

    if (irq >= NR_IRQS)
        return;

    /* unmask the irq*/
    status = readw(PL_INT_MASK);

    /* Check level 2 or level 3 interrupt */
    if (irq < MAX_LEVEL2_IRQ)
        writew(status | (1 << irq), PL_INT_MASK);
    else {
    /* Unmask DMA IRQ and Level 3 IRQ */
        writew(status | ( 1 << IRQ_PL_DMAC0), PL_INT_MASK);
        status = readw(PL_DMA_INT_MASK);
        writew((status | (1 << (irq - LEVEL3_IRQ_OFFSET))), PL_DMA_INT_MASK);
    }
}

static __inline__ void pl_dummy_mask_ack(unsigned int irq)
{
}


static char *mask2bit(unsigned short val, char hi)
{
    static char bitstr[32];
    int i, j;
    unsigned short mask;
    mask = 1 << 15;
    for (i = 0, j = 0; i < 16; i++, j++) {
        if (hi == 0)
            bitstr[j] = (val & mask ) ? (((15-i) > 9) ? ((15-i)-10)+'A' : (15-i)+'0') : '.';
        else
            bitstr[j] = (val & mask ) ? hi : '.';
        mask >>= 1;
        if (((i+1) % 8) == 0)
            bitstr[++j] = ' ';
    }
    return bitstr;
}


/*
 * The function will be invoked by arm/irq.c:get_irq_list()
 */
static char *p_mode[] = {
    "USR",  "FIQ",  "IRQ",  "SVC",  "  4",  "  5", "  6", "ABT",
    "  8",  "  9",  " 10",  "UDF",  " 12",  " 13", " 14", "SYS",
};

#define THUMB_FLAG      (1 << 5)
#define FIRQ_FLAG       (1 << 6)
#define IRQ_FLAG        (1 << 7)
#define OVER_FLAG       (1 << 28)
#define CARRY_FLAG      (1 << 29)
#define ZERO_FLAG       (1 << 30)
#define NEG_FLAG        (1 << 31)

int get_arch_irq_list(char *p)
{
    char *orgp = p;
    unsigned short intc_s, dmac_s;
    unsigned short intc_p, dmac_p;
    unsigned long cpsr, cr;
    extern unsigned long __fiqcnt;

    p += sprintf(p, "FIQ: %10lu\n", __fiqcnt);
    __asm__ __volatile__("mrs    %0, cpsr" :"=r"(cpsr));
    __asm__ __volatile__("mrc    p15, 0, %0, c1, c0" :"=r"(cr));
    p += sprintf(p, "\ncpsr:     %c%c%c%c---- %c%c%c[%s]  ",
        (cpsr & NEG_FLAG) ? 'N' : 'n',
        (cpsr & ZERO_FLAG) ? 'Z' : 'z',
        (cpsr & CARRY_FLAG) ? 'C' : 'c',
        (cpsr & OVER_FLAG) ? 'V' : 'v',
        (cpsr & IRQ_FLAG) ? 'I' : 'i',
        (cpsr & FIRQ_FLAG) ? 'F' : 'f',
        (cpsr & THUMB_FLAG) ? 'T' : 't',
        p_mode[cpsr & 0x0f]);

    p += sprintf(p, "cr:   %s%c%c%c%c%c%c %c%c%c%c%c%c%c%c\n",
        (cr & CR_RR) ? "RR" : "  ",     /* Round Robin cache replacement */
        (cr & CR_V) ?  'V' : ' ',       /* Vectors relocated to 0xffff0000 */
        (cr & CR_I) ?  'I' : ' ',       /* Icache enable */
        (cr & CR_Z) ?  'Z' : ' ',       /* Brench prediction */
        (cr & CR_F) ?  'F' : ' ',       /* Implementation defined */
        (cr & CD_R) ?  'R' : ' ',       /* ROM MMU protection bit */
        (cr & CR_S) ?  'S' : ' ',       /* System MMU protection */
        (cr & CD_B) ?  'B' : ' ',       /* Big endian */
        (cr & CR_L) ?  'L' : ' ',       /* Abort model selected */
        (cr & CR_D) ?  'D' : ' ',       /* 32 bit data address range */
        (cr & CR_P) ?  'P' : ' ',       /* 32 bit exception handler */
        (cr & CR_W) ?  'W' : ' ',       /* Write buffer enable */
        (cr & CR_C) ?  'C' : ' ',       /* Dcache enable */
        (cr & CR_A) ?  'A' : ' ',       /* Alignment abort enable */
        (cr & CR_M) ?  'M' : ' '        /* MMU enable */
    );


    intc_s = readw(PL_INT_MASK);
    dmac_s = readw(PL_DMA_INT_MASK);
    intc_p = readw(PL_INT_STATUS);
    dmac_p = readw(PL_DMA_INT_STATUS);

    p += sprintf(p, "mask int: %s ", mask2bit(intc_s, 0));
    p += sprintf(p, "dma: %s\n", mask2bit(dmac_s, 0));
    p += sprintf(p, "pend int: %s ", mask2bit(~intc_p, 'P'));
    p += sprintf(p, "dma: %s\n", mask2bit(~dmac_p, 'P'));

    return p - orgp;
}


/*
 * The function will be invoked by arm/kernel/irq.c:init_IRQ()
 */
void __init pl_init_irq(void)
{
    extern struct irqdesc irq_desc[];
    unsigned long flags;
    int i;

    for (i = 0; i < NR_IRQS; i++) {
        irq_desc[i].valid	= 1;
        irq_desc[i].probe_ok	= 1;
        irq_desc[i].mask	= pl_mask_irq;
        irq_desc[i].mask_ack    = pl_mask_irq ;
        irq_desc[i].unmask	= pl_unmask_irq;
    }

#if 0
    irq_desc[IRQ_PL_CONSOLE].mask_ack = pl_dummy_mask_ack;  // by Jim Lee(2005/03/25), let console intr can re-entry
#endif
    irq_desc[IRQ_PL_SD].noautoenable = 1;
    irq_desc[IRQ_PL_MS].noautoenable = 1;


    save_flags_cli(flags);

    /* disable all INTc and DMAc interrupt */
    writew(0x0, PL_INT_MASK);
    writew(0x0, PL_DMA_INT_MASK);

    restore_flags(flags);
}
