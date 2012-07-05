/*
 * linux/include/asm/arch/pcmcia.h
 *
 * Copyright (C) 2000 John G Dorsey <john+@cs.cmu.edu>
 *
 * This file contains definitions for the low-level SA-1100 kernel PCMCIA
 * interface. Please see linux/Documentation/arm/SA1100/PCMCIA for details.
 *
 * Modified to save in linux/arch/mips/pl1061/pcmcia.h
 * by Jim Lee(2002/09/12)
 * Modified by chun-wen 2003/03/07
 */

#ifndef _ASM_ARCH_PCMCIA
#define _ASM_ARCH_PCMCIA

#define PL1061_PCMCIA_MAX_SOCK   1

#ifndef __ASSEMBLY__

struct pcmcia_init {
  void (*handler)(int irq, void *dev, struct pt_regs *regs);
};

struct pcmcia_state {
  unsigned detect: 1,
            ready: 1,
             bvd1: 1,
             bvd2: 1,
           wrprot: 1,
            vs_3v: 1,
            vs_Xv: 1;
};

struct pcmcia_state_array {
  unsigned int size;
  struct pcmcia_state *state;
};

struct pcmcia_configure {
  unsigned sock: 8,
            vcc: 8,
            vpp: 8,
         output: 1,
        speaker: 1,
          reset: 1;
};

struct pcmcia_irq_info {
  unsigned int sock;
  unsigned int irq;
};

struct pcmcia_low_level {
  int (*init)(struct pcmcia_init *);
  int (*shutdown)(void);
  int (*socket_state)(struct pcmcia_state_array *);
  int (*get_irq_info)(struct pcmcia_irq_info *);
  int (*configure_socket)(const struct pcmcia_configure *);
};

extern struct pcmcia_low_level *pcmcia_low_level;

#endif  /* __ASSEMBLY__ */

#define PCMCIA_IO_0_BASE 0
#define PCMCIA_IO_1_BASE 0
#define rCF_BASE                VIRT_IO_ADDRESS(0x195C0000)
#define rCF_STATUS              ( rCF_BASE + 0x1f7 )
#define rCF_CIS_OFFSET          0x8000
#define CF_STATUS_READY         0x50

#endif
