/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 1999 by Ralf Baechle
 * Copyright (C) 1995, 1996 Paul M. Antoine
 * Copyright (C) 1998 Ulf Carlsson
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (C) 2002  Maciej W. Rozycki
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/spinlock.h>

#include <asm/bootinfo.h>
#include <asm/branch.h>
#include <asm/cpu.h>
#include <asm/module.h>
#include <asm/pgtable.h>
#include <asm/io.h>
#include <asm/ptrace.h>
#include <asm/watch.h>
#include <asm/system.h>
#include <asm/traps.h>
#include <asm/uaccess.h>
#include <asm/mmu_context.h>
#include <asm/cachectl.h>

extern asmlinkage void __xtlb_mod(void);
extern asmlinkage void __xtlb_tlbl(void);
extern asmlinkage void __xtlb_tlbs(void);
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

extern int fpu_emulator_cop1Handler(struct pt_regs *);
void fpu_emulator_init_fpu(void);

char watch_available = 0;
char dedicated_iv_available = 0;

int (*be_board_handler)(struct pt_regs *regs, int is_fixup);

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
void show_stack(unsigned long *sp)
{
	int i;
	unsigned long *stack;

	stack = sp;
	i = 0;

	printk("Stack:");
	while ((unsigned long) stack & (PAGE_SIZE - 1)) {
		unsigned long stackdata;

		if (__get_user(stackdata, stack++)) {
			printk(" (Bad stack address)");
			break;
		}

		printk(" %016lx", stackdata);

		if (++i > 40) {
			printk(" ...");
			break;
		}

		if (i % 4 == 0)
			printk("\n      ");
	}
}

void show_trace(unsigned long *sp)
{
	int i;
	unsigned long *stack;
	unsigned long kernel_start, kernel_end;
	unsigned long module_start, module_end;
	extern char _stext, _etext;

	stack = sp;
	i = 0;

	kernel_start = (unsigned long) &_stext;
	kernel_end = (unsigned long) &_etext;
	module_start = VMALLOC_START;
	module_end = module_start + MODULE_RANGE;

	printk("\nCall Trace:");

	while ((unsigned long) stack & (PAGE_SIZE - 1)) {
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

			/* Since our kernel is still at KSEG0,
			 * truncate the address so that ksymoops
			 * understands it.
			 */
			printk(" [<%08x>]", (unsigned int) addr);
			if (++i > 40) {
				printk(" ...");
				break;
			}
		}
	}
}

void show_trace_task(struct task_struct *tsk)
{
	show_trace((unsigned long *)tsk->thread.reg29);
}


void show_code(unsigned int *pc)
{
	long i;

	printk("\nCode:");

	for(i = -3 ; i < 6 ; i++) {
		unsigned int insn;
		if (__get_user(insn, pc + i)) {
			printk(" (Bad address in epc)\n");
			break;
		}
		printk("%c%08x%c",(i?' ':'<'),insn,(i?' ':'>'));
	}
}

void show_regs(struct pt_regs *regs)
{
	printk("Cpu %d\n", smp_processor_id());
	/* Saved main processor registers. */
	printk("$0      : %016lx %016lx %016lx %016lx\n",
	       0UL, regs->regs[1], regs->regs[2], regs->regs[3]);
	printk("$4      : %016lx %016lx %016lx %016lx\n",
               regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7]);
	printk("$8      : %016lx %016lx %016lx %016lx\n",
	       regs->regs[8], regs->regs[9], regs->regs[10], regs->regs[11]);
	printk("$12     : %016lx %016lx %016lx %016lx\n",
               regs->regs[12], regs->regs[13], regs->regs[14], regs->regs[15]);
	printk("$16     : %016lx %016lx %016lx %016lx\n",
	       regs->regs[16], regs->regs[17], regs->regs[18], regs->regs[19]);
	printk("$20     : %016lx %016lx %016lx %016lx\n",
               regs->regs[20], regs->regs[21], regs->regs[22], regs->regs[23]);
	printk("$24     : %016lx %016lx\n",
	       regs->regs[24], regs->regs[25]);
	printk("$28     : %016lx %016lx %016lx %016lx\n",
	       regs->regs[28], regs->regs[29], regs->regs[30], regs->regs[31]);
	printk("Hi      : %016lx\n", regs->hi);
	printk("Lo      : %016lx\n", regs->lo);

	/* Saved cp0 registers. */
	printk("epc     : %016lx    %s\nbadvaddr: %016lx\n",
	       regs->cp0_epc, print_tainted(), regs->cp0_badvaddr);
	printk("Status  : %08x  [ ", (unsigned int) regs->cp0_status);
	if (regs->cp0_status & ST0_KX) printk("KX ");
	if (regs->cp0_status & ST0_SX) printk("SX ");
	if (regs->cp0_status & ST0_UX) printk("UX ");
	switch (regs->cp0_status & ST0_KSU) {
		case KSU_USER: printk("USER ");			break;
		case KSU_SUPERVISOR: printk("SUPERVISOR ");	break;
		case KSU_KERNEL: printk("KERNEL ");		break;
		default: printk("BAD_MODE ");			break;
	}
	if (regs->cp0_status & ST0_ERL) printk("ERL ");
	if (regs->cp0_status & ST0_EXL) printk("EXL ");
	if (regs->cp0_status & ST0_IE) printk("IE ");
	printk("]\n");

	printk("Cause   : %08x\n", (unsigned int) regs->cp0_cause);
}

static spinlock_t die_lock;

void die(const char * str, struct pt_regs * regs)
{
	if (user_mode(regs))	/* Just return if in user mode.  */
		return;

	console_verbose();
	spin_lock_irq(&die_lock);
	printk("%s\n", str);
	show_regs(regs);
	printk("Process %s (pid: %d, stackpage=%08lx)\n",
		current->comm, current->pid, (unsigned long) current);
	show_stack((unsigned long *) regs->regs[29]);
	show_trace((unsigned long *) regs->regs[29]);
	show_code((unsigned int *) regs->cp0_epc);
	printk("\n");
	spin_unlock_irq(&die_lock);
	do_exit(SIGSEGV);
}

void die_if_kernel(const char * str, struct pt_regs * regs)
{
	if (!user_mode(regs))
		die(str, regs);
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

extern spinlock_t modlist_lock;

static inline unsigned long
search_dbe_table(unsigned long addr)
{
	unsigned long ret = 0;

#ifndef CONFIG_MODULES
	/* There is only the kernel to search.  */
	ret = search_one_table(__start___dbe_table, __stop___dbe_table-1, addr);
	return ret;
#else
	unsigned long flags;

	/* The kernel is the last "module" -- no need to treat it special.  */
	struct module *mp;
	struct archdata *ap;

	spin_lock_irqsave(&modlist_lock, flags);
	for (mp = module_list; mp != NULL; mp = mp->next) {
		if (!mod_member_present(mp, archdata_end) ||
        	    !mod_archdata_member_present(mp, struct archdata,
						 dbe_table_end))
			continue;
		ap = (struct archdata *)(mp->archdata_start);

		if (ap->dbe_table_start == NULL ||
		    !(mp->flags & (MOD_RUNNING | MOD_INITIALIZING)))
			continue;
		ret = search_one_table(ap->dbe_table_start,
				       ap->dbe_table_end - 1, addr);
		if (ret)
			break;
	}
	spin_unlock_irqrestore(&modlist_lock, flags);
	return ret;
#endif
}

asmlinkage void do_be(struct pt_regs *regs)
{
	unsigned long new_epc;
	unsigned long fixup = 0;
	int data = regs->cp0_cause & 4;
	int action = MIPS_BE_FATAL;

	if (data && !user_mode(regs))
		fixup = search_dbe_table(regs->cp0_epc);

	if (fixup)
		action = MIPS_BE_FIXUP;

	if (be_board_handler)
		action = be_board_handler(regs, fixup != 0);

	switch (action) {
	case MIPS_BE_DISCARD:
		return;
	case MIPS_BE_FIXUP:
		if (fixup) {
			new_epc = fixup_exception(dpf_reg, fixup,
						  regs->cp0_epc);
			regs->cp0_epc = new_epc;
			return;
		}
		break;
	default:
		break;
	}

	/*
	 * Assume it would be too dangerous to continue ...
	 */
	printk(KERN_ALERT "%s bus error, epc == %08lx, ra == %08lx\n",
	       data ? "Data" : "Instruction",
	       regs->cp0_epc, regs->regs[31]);
	die_if_kernel("Oops", regs);
	force_sig(SIGBUS, current);
}

void do_ov(struct pt_regs *regs)
{
	if (compute_return_epc(regs))
		return;
	force_sig(SIGFPE, current);
}

/*
 * XXX Delayed fp exceptions when doing a lazy ctx switch XXX
 */
asmlinkage void do_fpe(struct pt_regs *regs, unsigned long fcr31)
{
	if (fcr31 & FPU_CSR_UNI_X) {
		int sig;

		/*
	 	 * Unimplemented operation exception.  If we've got the full
		 * software emulator on-board, let's use it...
		 *
		 * Force FPU to dump state into task/thread context.  We're
		 * moving a lot of data here for what is probably a single
		 * instruction, but the alternative is to pre-decode the FP
		 * register operands before invoking the emulator, which seems
		 * a bit extreme for what should be an infrequent event.
		 */
		save_fp(current);

		/* Run the emulator */
		sig = fpu_emulator_cop1Handler(regs);

		/* 
		 * We can't allow the emulated instruction to leave any of 
		 * the cause bit set in $fcr31.
		 */
		current->thread.fpu.soft.sr &= ~FPU_CSR_ALL_X;

		/* Restore the hardware register state */
		restore_fp(current);

		/* If something went wrong, signal */
		if (sig)
		{
			/* 
			 * Return EPC is not calculated in the FPU emulator, 
			 * if a signal is being send. So we calculate it here.
			 */
			compute_return_epc(regs);
			force_sig(sig, current);
		}

		return;
	}

	if (compute_return_epc(regs))
		return;
	force_sig(SIGFPE, current);
}

void do_bp(struct pt_regs *regs)
{
	unsigned int opcode, bcode;
	unsigned int *epc;
	siginfo_t info;

	epc = (unsigned int *) regs->cp0_epc +
	      ((regs->cp0_cause & CAUSEF_BD) != 0);
	if (get_user(opcode, epc))
		goto sigsegv;

	/*
	 * There is the ancient bug in the MIPS assemblers that the break
	 * code starts left to bit 16 instead to bit 6 in the opcode.
	 * Gas is bug-compatible ...
	 */
	bcode = ((opcode >> 16) & ((1 << 20) - 1));

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
		info.si_addr = (void *)regs->cp0_epc;
		force_sig_info(SIGFPE, &info, current);
		break;
	default:
		force_sig(SIGTRAP, current);
	}

	force_sig(SIGTRAP, current);
	return;

sigsegv:
	force_sig(SIGSEGV, current);
}

void do_tr(struct pt_regs *regs)
{
	unsigned int opcode, bcode;
	unsigned int *epc;
	siginfo_t info;

	epc = (unsigned int *) regs->cp0_epc +
	      ((regs->cp0_cause & CAUSEF_BD) != 0);
	if (get_user(opcode, epc))
		goto sigsegv;

	bcode = ((opcode >> 6) & ((1 << 20) - 1));

	/*
	 * (A short test says that IRIX 5.3 sends SIGTRAP for all break
	 * insns, even for break codes that indicate arithmetic failures.
	 * Wiered ...)
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
		info.si_addr = (void *)regs->cp0_epc;
		force_sig_info(SIGFPE, &info, current);
		break;
	default:
		force_sig(SIGTRAP, current);
	}
	return;

sigsegv:
	force_sig(SIGSEGV, current);
}

void do_ri(struct pt_regs *regs)
{
	if (compute_return_epc(regs))
		return;

	force_sig(SIGILL, current);
}

void do_cpu(struct pt_regs *regs)
{
	u32 cpid;
	int sig;

	cpid = (regs->cp0_cause >> CAUSEB_CE) & 3;
	if (cpid != 1)
		goto bad_cid;

	if (!(mips_cpu.options & MIPS_CPU_FPU))
		goto fp_emul;

	regs->cp0_status |= ST0_CU1;

#ifdef CONFIG_SMP
	if (current->used_math) {
		lazy_fpu_switch(0, current);
	} else {
		init_fpu();
		current->used_math = 1;
	}
	current->flags |= PF_USEDFPU;
#else
	if (last_task_used_math == current)
		return;

	if (current->used_math) {		/* Using the FPU again.  */
		lazy_fpu_switch(last_task_used_math, current);
	} else {				/* First time FPU user.  */
		lazy_fpu_switch(last_task_used_math, 0);
		init_fpu();
		current->used_math = 1;
	}
	last_task_used_math = current;
#endif
	return;

fp_emul:
	if (!current->used_math) {
		fpu_emulator_init_fpu();
		current->used_math = 1;
	}
	sig = fpu_emulator_cop1Handler(regs);
	if (sig)
	{
		/* 
		 * Return EPC is not calculated in the FPU emulator, if 
		 * a signal is being send. So we calculate it here.
		 */
		compute_return_epc(regs);
		force_sig(sig, current);
	}
	return;

bad_cid:
	compute_return_epc(regs);
	force_sig(SIGILL, current);
}

void do_watch(struct pt_regs *regs)
{
	/*
	 * We use the watch exception where available to detect stack
	 * overflows.
	 */
	show_regs(regs);
	panic("Caught WATCH exception - probably caused by stack overflow.");
}

asmlinkage void do_mcheck(struct pt_regs *regs)
{
	show_regs(regs);
	panic("Caught Machine Check exception - probably caused by multiple "
	      "matching entries in the TLB.");
}

void do_reserved(struct pt_regs *regs)
{
	/*
	 * Game over - no way to handle this if it ever occurs.  Most probably
	 * caused by a new unknown cpu type or after another deadly
	 * hard/software error.
	 */
	panic("Caught reserved exception %ld - should not happen.",
	      (regs->cp0_cause & 0x1f) >> 2);
}

static inline void watch_init(unsigned long cputype)
{
	switch(cputype) {
	case CPU_R10000:
	case CPU_R4000MC:
	case CPU_R4400MC:
	case CPU_R4000SC:
	case CPU_R4400SC:
	case CPU_R4000PC:
	case CPU_R4400PC:
	case CPU_R4200:
	case CPU_R4300:
		set_except_vector(23, handle_watch);
		watch_available = 1;
		break;
	}
}

unsigned long exception_handlers[32];

/*
 * As a side effect of the way this is implemented we're limited
 * to interrupt handlers in the address range from
 * KSEG0 <= x < KSEG0 + 256mb on the Nevada.  Oh well ...
 */
void set_except_vector(int n, void *addr)
{
	unsigned long handler = (unsigned long) addr;
	exception_handlers[n] = handler;

	if (n == 0 && mips_cpu.options & MIPS_CPU_DIVEC) {
		*(volatile u32 *)(KSEG0+0x200) = 0x08000000 |
		                                 (0x03ffffff & (handler >> 2));
		flush_icache_range(KSEG0+0x200, KSEG0 + 0x204);
	}
}

void __init per_cpu_trap_init(void)
{
	unsigned int cpu = smp_processor_id();

	/* Some firmware leaves the BEV flag set, clear it.  */
	clear_cp0_status(ST0_CU1|ST0_CU2|ST0_CU3|ST0_BEV);
	set_cp0_status(ST0_CU0|ST0_FR|ST0_KX|ST0_SX|ST0_UX);

	/*
	 * Some MIPS CPUs have a dedicated interrupt vector which reduces the
	 * interrupt processing overhead.  Use it where available.
	 */
	if (mips_cpu.options & MIPS_CPU_DIVEC)
		set_cp0_cause(CAUSEF_IV);

	cpu_data[cpu].asid_cache = ASID_FIRST_VERSION;
	set_context(((long)(&pgd_current[cpu])) << 23);
	set_wired(0);
}

void __init trap_init(void)
{
	extern char except_vec0;
	extern char except_vec1_r10k;
	extern char except_vec2_generic;
	extern char except_vec3_generic, except_vec3_r4000;
	extern char except_vec4;
	unsigned long i;
	int dummy;

	per_cpu_trap_init();

	/* Copy the generic exception handlers to their final destination. */
	memcpy((void *)(KSEG0 + 0x100), &except_vec2_generic, 0x80);
	memcpy((void *)(KSEG0 + 0x180), &except_vec3_generic, 0x80);

	/*
	 * Setup default vectors
	 */
	for(i = 0; i <= 31; i++)
		set_except_vector(i, handle_reserved);

	/*
	 * Only some CPUs have the watch exceptions or a dedicated
	 * interrupt vector.
	 */
	watch_init(mips_cpu.cputype);

	/*
	 * Some MIPS CPUs have a dedicated interrupt vector which reduces the
	 * interrupt processing overhead.  Use it where available.
	 */
	memcpy((void *)(KSEG0 + 0x200), &except_vec4, 8);

	if (mips_cpu.options & MIPS_CPU_MCHECK)
		set_except_vector(24, handle_mcheck);

	/*
	 * The Data Bus Errors / Instruction Bus Errors are signaled
	 * by external hardware.  Therefore these two exceptions
	 * may have board specific handlers.
	 */
	bus_error_init();

	/*
	 * Handling the following exceptions depends mostly of the cpu type
	 */
	switch(mips_cpu.cputype) {
        case CPU_SB1:
#ifdef CONFIG_SB1_CACHE_ERROR
		{
		/* Special cache error handler for SB1 */
		extern char except_vec2_sb1;
		memcpy((void *)(KSEG0 + 0x100), &except_vec2_sb1, 0x80);
		memcpy((void *)(KSEG1 + 0x100), &except_vec2_sb1, 0x80);
		}
#endif
		/* Enable timer interrupt and scd mapped interrupt */
		clear_cp0_status(0xf000);
		set_cp0_status(0xc00);

		/* Fall through. */
	case CPU_R10000:
	case CPU_R4000MC:
	case CPU_R4400MC:
	case CPU_R4000SC:
	case CPU_R4400SC:
	case CPU_R4000PC:
	case CPU_R4400PC:
	case CPU_R4200:
	case CPU_R4300:
	case CPU_R4600:
	case CPU_R5000:
	case CPU_NEVADA:
		/* Debug TLB refill handler.  */
		memcpy((void *)KSEG0, &except_vec0, 0x80);
		memcpy((void *)KSEG0 + 0x080, &except_vec1_r10k, 0x80);

		if (mips_cpu.options & MIPS_CPU_VCE) {
			memcpy((void *)(KSEG0 + 0x180), &except_vec3_r4000,
			       0x80);
		} else {
			memcpy((void *)(KSEG0 + 0x180), &except_vec3_generic,
			       0x80);
		}

		set_except_vector(1, __xtlb_mod);
		set_except_vector(2, __xtlb_tlbl);
		set_except_vector(3, __xtlb_tlbs);
		set_except_vector(4, handle_adel);
		set_except_vector(5, handle_ades);

		set_except_vector(6, handle_ibe);
		set_except_vector(7, handle_dbe);

		set_except_vector(8, handle_sys);
		set_except_vector(9, handle_bp);
		set_except_vector(10, handle_ri);
		set_except_vector(11, handle_cpu);
		set_except_vector(12, handle_ov);
		set_except_vector(13, handle_tr);
		set_except_vector(15, handle_fpe);
		break;

	case CPU_R8000:
		panic("R8000 is unsupported");
		break;

	case CPU_UNKNOWN:
	default:
		panic("Unknown CPU type");
	}
	flush_icache_range(KSEG0, KSEG0 + 0x200);

	if (mips_cpu.isa_level == MIPS_CPU_ISA_IV)
		set_cp0_status(ST0_XX);

	atomic_inc(&init_mm.mm_count);	/* XXX UP?  */
	current->active_mm = &init_mm;
}
