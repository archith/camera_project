/*
 * Setup the interrupt stuff.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 */
#include <linux/config.h>

#if defined(CONFIG_ARCH_PL1071) || defined(CONFIG_ARCH_PL1091)

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/param.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/console.h>

#include <asm/irq.h>
#include <asm/pl_reg.h>
#include <asm/arch-pl1091/reboot.h>

#ifdef CONFIG_PC_KEYB
#include <asm/arch/keyboard.h>
extern struct kbd_ops no_kbd_ops;
struct kbd_ops *kbd_ops;
#endif

extern void pl_machine_restart(char *command);
extern void pl_machine_halt(void);
extern void pl_machine_power_off(void);

void (*board_time_init) (struct irqaction *irq);

/*
 * enable the periodic interrupts
 */
extern void pl_time_init(struct irqaction *);

void __init pl_setup(void)
{
    board_time_init = pl_time_init;

    _machine_restart = pl_machine_restart;
    _machine_halt = pl_machine_halt;
    _machine_power_off = pl_machine_power_off;

#if defined(CONFIG_VT) && defined(CONFIG_FB)
    conswitchp = &dummy_con;
#endif

#ifdef CONFIG_PC_KEYB
	kbd_ops = &no_kbd_ops;
#endif

}

#else

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/param.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/bootmem.h>

#include <linux/console.h>

#include <asm/mipsregs.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/reboot.h>

#include <asm/addrspace.h>
#include <asm/pl_reg.h>

/*
 * Information regarding the IRQ Controller
 *
 * isr and imr are also hardcoded for different machines in int_handler.S
 */


int __init page_is_ram(unsigned long pagenr)
{
    return 1;
}


char arcs_cmdline[CL_SIZE];

extern void pl_machine_restart(char *command);
extern void pl_machine_halt(void);
extern void pl_machine_power_off(void);


void (*board_time_init) (struct irqaction *irq);

extern void pl_irq_setup(void);
/*
 * The board specific setup routine sets irq_setup to point to a board
 * specific setup routine.
 */
extern void (*irq_setup)(void);

/*
 * enable the periodic interrupts
 */
extern void pl_time_init(struct irqaction *);

void __init pl_setup(void)
{
    irq_setup = pl_irq_setup;

    board_time_init = pl_time_init;

    _machine_restart = pl_machine_restart;
    _machine_halt = pl_machine_halt;
    _machine_power_off = pl_machine_power_off;

#if defined(CONFIG_VT) && defined(CONFIG_FB)

    conswitchp = &dummy_con;
#endif
}

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_ALIGN(x)	(((unsigned long)(x) + (PAGE_SIZE - 1)) & PAGE_MASK)

#define DEFAULT_FBMEM_SIZE      (1024*1024)

static unsigned long inline size2val(unsigned long size, unsigned long unit)
{
    int i;
    size = size /unit;
    for(i=0 ; size > 1; i++)
        size >>= 1;
    return i;
}


int __init prom_init(int argc, char **argv, char **envp, char *promv)
{
    unsigned int ram_map;
	unsigned long free_start, free_end, start_pfn, bootmap_size;
    unsigned long mem_size;
    unsigned long fbmem_size = 0;
    unsigned long fbmem_val;
    unsigned long kernel_val;
    unsigned long mem_val;
    extern unsigned int _end;
    extern int initrd_below_start_ok;

    mips_machgroup = MACH_GROUP_PROLIFIC;
    mips_machtype = MACH_PL_1061;

    /* get memory size */
#ifdef CONFIG_FB
    fbmem_size = DEFAULT_FBMEM_SIZE;
#endif

    ram_map = wordAddr(rMEMORY_MAP);

    mem_size = ram_map & RAM_SIZE_MASK;

    /* resize memory */
    fbmem_val = size2val(fbmem_size, 4*1024) << 8;
    mem_val   = (size2val(mem_size, 2*1024*1024) | 0x08) << 4;
    kernel_val = size2val(mem_size, 2*1024*1024) | 0x08;
    wordAddr(rMEMORY_MAP) = (ram_map & ~RESIZING_MASK) | 
            fbmem_val | mem_val | kernel_val;


    mem_size -= fbmem_size; /* reserved framebuffer from kernel */   

    free_start = PHYSADDR(PFN_ALIGN(&_end));
    free_end = mem_size;
    start_pfn =  PFN_UP(PHYSADDR((unsigned long) &_end));

    /* Register all the contiguous memory with the bootmem allocator
       and free it.  Be careful about the bootmem freemap. */
    bootmap_size = init_bootmem(start_pfn, mem_size >> PAGE_SHIFT);

    /* Free the entire available memory after the _end symbol. */
    free_start += bootmap_size;
    free_bootmem(free_start, free_end - free_start);

    initrd_below_start_ok = 0;

#ifdef CONFIG_PL_PROM_CONSOLE
    pl_console_init();
#endif

    return 0;
}


#endif

