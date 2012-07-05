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
/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 1999 by Ralf Baechle
 * Modified for R3000 by Paul M. Antoine, 1995, 1996
 * Complete output from die() by Ulf Carlsson, 1998
 * Copyright (C) 1999 Silicon Graphics, Inc.
 *
 * Kevin D. Kissell, kevink@mips.com and Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
 *
 * Jim Lee. Prolific Technology Inc.
 * 2002/07/11
 * revised for PL1091 & uClinux-dist
 * 2003/05/23
 */

/* Because lots of mips dependence, so need to port it carefully */
/* by Jim Lee(2002/07/31) */
/* For Example, We use the trap_init() which declare in armnommu/kernel/traps.c */
/*  instead of the one declared in this file */
#include <linux/config.h>

#if defined(CONFIG_ARCH_PL1071) || defined(CONFIG_ARCH_PL1091)
unsigned long exception_handlers[32];

void set_except_vector(int n, void *addr)
{
	unsigned handler = (unsigned long) addr;
	exception_handlers[n] = handler;
}


#else
#include <linux/config.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/spinlock.h>

#include <asm/bootinfo.h>
#include <asm/branch.h>
#include <asm/cpu.h>
#include <asm/cachectl.h>
#include <asm/inst.h>
#include <asm/jazz.h>
#include <asm/pgtable.h>
#include <asm/io.h>
#include <asm/siginfo.h>
#include <asm/watch.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/mmu_context.h>

/*
 * Machine specific interrupt handlers
 */
extern asmlinkage void acer_pica_61_handle_int(void);
extern asmlinkage void decstation_handle_int(void);
extern asmlinkage void deskstation_rpc44_handle_int(void);
extern asmlinkage void deskstation_tyne_handle_int(void);
extern asmlinkage void mips_magnum_4000_handle_int(void);

extern asmlinkage void handle_mod(void);
extern asmlinkage void handle_tlbl(void);
extern asmlinkage void handle_tlbs(void);
extern asmlinkage void handle_adel(void);
extern asmlinkage void handle_ades(void);
extern asmlinkage void handle_ibe(void);
extern asmlinkage void handle_dbe(void);
extern asmlinkage void handle_sys(void);
extern asmlinkage void handle_bp(void);
extern asmlinkage void handle_ri(void);
extern asmlinkage void handle_cpu(void);
extern asmlinkage void handle_ov(void);
extern asmlinkage void handle_tr(void);
extern asmlinkage void handle_fpe(void);
extern asmlinkage void handle_watch(void);
extern asmlinkage void handle_mcheck(void);
extern asmlinkage void handle_reserved(void);

extern int fpu_emulator_cop1Handler(int, struct pt_regs *);

/* static char *cpu_names[] = CPU_NAMES; */

char watch_available = 0;
char dedicated_iv_available = 0;
char vce_available = 0;

void (*ibe_board_handler)(struct pt_regs *regs);
void (*dbe_board_handler)(struct pt_regs *regs);

int kstack_depth_to_print = 24;

/*
 * These constant is for searching for possible module text segments.
 * MODULE_RANGE is a guess of how much space is likely to be vmalloced.
 */
#define MODULE_RANGE (8*1024*1024)

/*
 * This routine abuses get_user()/put_user() to reference pointers
 * with at least a bit of error checking ...
 */
void show_stack(unsigned int *sp)
{
	int i;
	unsigned int *stack;

	stack = sp;
	i = 0;

	printk("Stack:");
	while ((unsigned long) stack & (PAGE_SIZE - 1)) {
		unsigned long stackdata;

		if (__get_user(stackdata, stack++)) {
			printk(" (Bad stack address)");
			break;
		}

		printk(" %08lx", stackdata);

		if (++i > 40) {
			printk(" ...");
			break;
		}

		if (i % 8 == 0)
			printk("\n      ");
	}
}

void show_trace(unsigned int *sp)
{
	int i;
	unsigned int *stack;
	unsigned long kernel_start, kernel_end;
	unsigned long module_start, module_end;
	extern char _stext, _etext;

	stack = sp;
	i = 0;

	kernel_start = (unsigned long) &_stext;
	kernel_end = (unsigned long) &_etext;
//daret
	module_start = KSEG1; // VMALLOC_START;	
	module_end = module_start + MODULE_RANGE;

	printk("\nTrace:");

	while ((unsigned long) stack & (PAGE_SIZE -1)) {
		unsigned long addr;

		if (__get_user(addr, stack++)) {
			printk(" (Bad stack address)\n");
			break;
		}

		/*
		 * If the address is either in the text segment of the
		 * kernel, or in the region which contains vmalloc'ed
		 * memory, it *may* be the address of a calling
		 * routine; if so, print it so that someone tracing
		 * down the cause of the crash will be able to figure
		 * out the call path that was taken.
		 */

		if ((addr >= kernel_start && addr < kernel_end) ||
		    (addr >= module_start && addr < module_end)) { 

			printk(" [%08lx]", addr);
			if (++i > 40) {
				printk(" ...");
				break;
			}

    		if (i % 4 == 0)
	    		printk("\n      ");
		}
	}
}

void show_code(unsigned int *pc)
{
	long i;

	printk("\nCode:");

	for(i = -3 ; i < 6 ; i++) {

		unsigned long insn;
		if (__get_user(insn, pc + i)) {
			printk(" (Bad address in epc)\n");
			break;
		}
		printk("%c%08lx%c",(i?' ':'<'),insn,(i?' ':'>'));
	}
}

spinlock_t die_lock;

extern void __die(const char * str, struct pt_regs * regs, const char *where,
                  unsigned long line)
{
	console_verbose();
	spin_lock_irq(&die_lock);
	printk("%s", str);
	if (where)
		printk(" in %s, line %ld", where, line);
	printk(":\n");
	show_regs(regs);
	printk("Process %s (pid: %d, stackpage=%08lx)\n",
		current->comm, current->pid, (unsigned long) current);
	show_stack((unsigned int *) regs->regs[29]);
	show_trace((unsigned int *) regs->regs[29]);
	show_code((unsigned int *) regs->cp0_epc);
	printk("\n");
while(1);
	spin_unlock_irq(&die_lock);
	do_exit(SIGSEGV);
}

void __die_if_kernel(const char * str, struct pt_regs * regs, const char *where,
	unsigned long line)
{
	if (!user_mode(regs))
		__die(str, regs, where, line);
}

extern const struct exception_table_entry __start___dbe_table[];
extern const struct exception_table_entry __stop___dbe_table[];

void __declare_dbe_table(void)
{
	__asm__ __volatile__(
	".section\t__dbe_table,\"a\"\n\t"
	".previous"
	);
}

static inline unsigned long
search_one_table(const struct exception_table_entry *first,
		 const struct exception_table_entry *last,
		 unsigned long value)
{
	const struct exception_table_entry *mid;
	long diff;

	while (first < last) {
		mid = (last - first) / 2 + first;
		diff = mid->insn - value;
		if (diff < 0)
			first = mid + 1;
		else
			last = mid;
	}
	return (first == last && first->insn == value) ? first->nextinsn : 0;
}

#define search_dbe_table(addr)	\
	search_one_table(__start___dbe_table, __stop___dbe_table - 1, (addr))

static void default_be_board_handler(struct pt_regs *regs)
{
	unsigned long new_epc;
	unsigned long fixup = search_dbe_table(regs->cp0_epc);

	if (fixup) {
		new_epc = fixup_exception(dpf_reg, fixup, regs->cp0_epc);
		regs->cp0_epc = new_epc;
		return;
	}

	/*
	 * Assume it would be too dangerous to continue ...
	 */
/* XXX */
printk("Got Bus Error at %08x\n", (unsigned int)regs->cp0_epc);
show_regs(regs); while(1);
	force_sig(SIGBUS, current);
}

void do_pl_ade(struct pt_regs *regs)
{
    printk("[%s:%d] Address error at %08lx \n",
	    current->comm, current->pid, regs->cp0_epc);
    if (current->pid > 1)
        force_sig(SIGSEGV, current); 
    else
        __die("do_ade", regs, __FUNCTION__, __LINE__);

	if (compute_return_epc(regs)) 
        return;
}

asmlinkage
void do_adel(struct pt_regs *regs)
{
    printk("Exception <AdEL> at %08x\n", (unsigned int)regs->cp0_epc);
    do_pl_ade(regs);
}

asmlinkage
void do_ades(struct pt_regs *regs)
{
    printk("Exception <AdES> at %08x\n", (unsigned int)regs->cp0_epc);
    do_pl_ade(regs);
}

asmlinkage
void do_ibe(struct pt_regs *regs)
{
	printk("Exception <IBE> at %08x\n", (unsigned int)regs->cp0_epc);
	ibe_board_handler(regs);
}

asmlinkage
void do_dbe(struct pt_regs *regs)
{
	printk("Exception <DBE> at %08x\n",(unsigned int)regs->cp0_epc);
	dbe_board_handler(regs);
}

asmlinkage
void do_ov(struct pt_regs *regs)
{
	printk("Exception <OV> at %08x\n",(unsigned int)regs->cp0_epc);
	if (compute_return_epc(regs))
		return;
	force_sig(SIGFPE, current);
}

#ifdef CONFIG_MIPS_FPE_MODULE
static void (*fpe_handler)(struct pt_regs *regs, unsigned int fcr31);

/*
 * Register_fpe/unregister_fpe are for debugging purposes only.  To make
 * this hack work a bit better there is no error checking.
 */
int register_fpe(void (*handler)(struct pt_regs *regs, unsigned int fcr31))
{
	fpe_handler = handler;
	return 0;
}

int unregister_fpe(void (*handler)(struct pt_regs *regs, unsigned int fcr31))
{
	fpe_handler = NULL;
	return 0;
}
#endif

/*
 * XXX Delayed fp exceptions when doing a lazy ctx switch XXX
 */

//daret
//deleted do_fpe body
asmlinkage
void do_fpe(struct pt_regs *regs, unsigned long fcr31)
{
    printk("Exception <FPE> at %08x\n",(unsigned int)regs->cp0_epc);
}

static inline int get_insn_opcode(struct pt_regs *regs, unsigned int *opcode)
{
	unsigned int *epc;

	epc = (unsigned int *) (unsigned long) regs->cp0_epc;
	if (regs->cp0_cause & CAUSEF_BD)
		epc += 4;

	if (verify_area(VERIFY_READ, epc, 4)) {
		force_sig(SIGSEGV, current);
		return 1;
	}
	*opcode = *epc;

	return 0;
}

asmlinkage
void do_bp(struct pt_regs *regs)
{
    printk("Exception <BP> at %08x :\n",(unsigned int)regs->cp0_epc);

	show_regs(regs);
	printk("Process %s (pid: %d, stackpage=%08lx)\n",
		current->comm, current->pid, (unsigned long) current);
	show_stack((unsigned int *) regs->regs[29]);
	show_trace((unsigned int *) regs->regs[29]);
	show_code((unsigned int *) regs->cp0_epc);
	printk("\n");

    if (current->pid != 0)
        force_sig(SIGTRAP, current);
    else
        regs->cp0_epc += 4;   /* next instruction */
}

asmlinkage
void do_tr(struct pt_regs *regs)
{
	siginfo_t info;
	unsigned int opcode, bcode;
	
	printk("Exception <TR> at %08x\n",(unsigned int)regs->cp0_epc);
	
	if (get_insn_opcode(regs, &opcode))
		return;
	bcode = ((opcode >> 6) & ((1 << 20) - 1));

	/*
	 * (A short test says that IRIX 5.3 sends SIGTRAP for all break
	 * insns, even for break codes that indicate arithmetic failures.
	 * Weird ...)
	 * But should we continue the brokenness???  --macro
	 */
	switch (bcode) {
	case 6:
	case 7:
		if (bcode == 7)
			info.si_code = FPE_INTDIV;
		else
			info.si_code = FPE_INTOVF;
		info.si_signo = SIGFPE;
		info.si_errno = 0;
		info.si_addr = (void *)compute_return_epc(regs);
		force_sig_info(SIGFPE, &info, current);
		break;
	default:
		force_sig(SIGTRAP, current);
	}
}


/*
 * userland emulation for R2300 CPUs
 * needed for the multithreading part of glibc
 */
extern int do_pl_ri(struct pt_regs *regs);
asmlinkage
void do_ri(struct pt_regs *regs)
{
	if (do_pl_ri(regs) == 0) {
        unsigned int opcode;
        get_insn_opcode(regs, &opcode);
	    printk("[%s:%d] Illegal instruction at %08lx ra=%08lx (%08x)\n",
	    current->comm, current->pid, regs->cp0_epc, regs->regs[31], opcode);
        if (current->pid > 1)
            force_sig(SIGILL, current); 
        else
            __die("do_ri", regs, __FUNCTION__, __LINE__);
    }

	if (compute_return_epc(regs)) 
        return;
}


/* Co-Process unsable exception */
asmlinkage
void do_cpu(struct pt_regs *regs)
{
    printk("Exception <CpU> at %08x\n",(unsigned int)regs->cp0_epc);
}

asmlinkage
void do_watch(struct pt_regs *regs)
{
	/*
	 * We use the watch exception where available to detect stack
	 * overflows.
	 */
	printk("Exception <WATCH> at %08x\n",(unsigned int)regs->cp0_epc);
	show_regs(regs);
	panic("Caught WATCH exception - probably caused by stack overflow.");
}

asmlinkage
void do_reserved(struct pt_regs *regs)
{
	/*
	 * Game over - no way to handle this if it ever occurs.
	 * Most probably caused by a new unknown cpu type or
	 * after another deadly hard/software error.
	 */
	printk("Exception <RESERVED> at %08x\n",(unsigned int)regs->cp0_epc);
	panic("Caught reserved exception - should not happen.");
}

static inline void watch_init(unsigned long cputype)
{
    /* R3000 don't support watch exception */
    /* do nothing */
}

/*
 * Some MIPS CPUs have a dedicated interrupt vector which reduces the
 * interrupt processing overhead.  Use it where available.
 * FIXME: more CPUs than just the Nevada have this feature.
 */
static inline void setup_dedicated_int(void)
{
    /* R3000 don't support dedicated interrupt */
    /* do nothing */
}

unsigned long exception_handlers[32];

/*
 * As a side effect of the way this is implemented we're limited
 * to interrupt handlers in the address range from
 * KSEG0 <= x < KSEG0 + 256mb on the Nevada.  Oh well ...
 */
void set_except_vector(int n, void *addr)
{
	unsigned handler = (unsigned long) addr;
	exception_handlers[n] = handler;
}

void __init trap_init(void)
{
	extern char except_vec_tlb;
	extern char except_vec_allother;
	unsigned long i;

	/* Some firmware leaves the BEV flag set, clear it.  */
	set_cp0_status(ST0_BEV, 0);


	/*
	 * Setup default vectors
	 */

	for(i = 0; i <= 31; i++)
		set_except_vector(i, handle_reserved);


	/*
	 * Only some CPUs have the watch exceptions or a dedicated
	 * interrupt vector.
	 */
	watch_init(mips_cputype);
	setup_dedicated_int();

#ifndef NO_MM
    set_except_vector(1, handle_mod);
    set_except_vector(2, handle_tlbl);
    set_except_vector(3, handle_tlbs);
#endif
	set_except_vector(4, handle_adel);
	set_except_vector(5, handle_ades);

    /* 
     * The Data Bus Error/ Instruction Bus Errors are signaled
     * by external hardware.  Therefore these two expection have
     * board specific handlers.
     */
    set_except_vector(6, handle_ibe);
    set_except_vector(7, handle_dbe);
    ibe_board_handler = default_be_board_handler;
    dbe_board_handler = default_be_board_handler;


	set_except_vector(8, handle_sys);
	set_except_vector(9, handle_bp);
	set_except_vector(10, handle_ri);
	set_except_vector(11, handle_cpu);
	set_except_vector(12, handle_ov);
    set_except_vector(13, handle_tr);
	set_except_vector(15, handle_fpe);
	

    memcpy((void *)(KSEG0), &except_vec_tlb, 0x80);
    memcpy((void *)(KSEG0 + 0x80), &except_vec_allother, 0x80);
	flush_icache_range(KSEG0, KSEG0 + 0x200);


	atomic_inc(&init_mm.mm_count);
	current->active_mm = &init_mm;
#ifndef NO_MM
    current_pgd = init_mm.pgd;
#endif
}
#endif /* CONFIG_PL1071 */
