/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1991, 1992  Linus Torvalds
 * Copyright (C) 1994 - 2000  Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 */
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/personality.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>

#include <asm/asm.h>
#include <asm/bitops.h>
#include <asm/cacheflush.h>
#include <asm/stackframe.h>
#include <asm/uaccess.h>
#include <asm/ucontext.h>
#include <asm/system.h>

#define DEBUG_SIG 0

#define _BLOCKABLE (~(sigmask(SIGKILL) | sigmask(SIGSTOP)))

extern asmlinkage int do_signal(sigset_t *oldset, struct pt_regs *regs);
extern asmlinkage int (*save_fp_context)(struct sigcontext *sc);
extern asmlinkage int (*restore_fp_context)(struct sigcontext *sc);

extern asmlinkage void do_syscall_trace(void);

/*
 * Atomically swap in the new signal mask, and wait for a signal.
 */
asmlinkage int sys_rt_sigsuspend(abi64_no_regargs, struct pt_regs regs)
{
	sigset_t *unewset, saveset, newset;
        size_t sigsetsize;

	save_static(&regs);

	/* XXX Don't preclude handling different sized sigset_t's.  */
	sigsetsize = regs.regs[5];
	if (sigsetsize != sizeof(sigset_t))
		return -EINVAL;

	unewset = (sigset_t *) regs.regs[4];
	if (copy_from_user(&newset, unewset, sizeof(newset)))
		return -EFAULT;
	sigdelsetmask(&newset, ~_BLOCKABLE);

	spin_lock_irq(&current->sigmask_lock);
	saveset = current->blocked;
	current->blocked = newset;
        recalc_sigpending();
	spin_unlock_irq(&current->sigmask_lock);

	regs.regs[2] = EINTR;
	regs.regs[7] = 1;
	while (1) {
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		if (do_signal(&saveset, &regs))
			return -EINTR;
	}
}

asmlinkage int sys_sigaltstack(abi64_no_regargs, struct pt_regs regs)
{
	const stack_t *uss = (const stack_t *) regs.regs[4];
	stack_t *uoss = (stack_t *) regs.regs[5];
	unsigned long usp = regs.regs[29];

	return do_sigaltstack(uss, uoss, usp);
}

static inline int restore_thread_fp_context(struct sigcontext *sc)
{
     u64 *pfreg = &current->thread.fpu.soft.regs[0];
     int err = 0;

     /*
      * Copy all 32 64-bit values.
      */

#define restore_fpr(i)                          \
     do { err |= __get_user(pfreg[i], &sc->sc_fpregs[i]); } while(0)

     restore_fpr( 0); restore_fpr( 1); restore_fpr( 2); restore_fpr( 3);
     restore_fpr( 4); restore_fpr( 5); restore_fpr( 6); restore_fpr( 7);
     restore_fpr( 8); restore_fpr( 9); restore_fpr(10); restore_fpr(11);
     restore_fpr(12); restore_fpr(13); restore_fpr(14); restore_fpr(15);
     restore_fpr(16); restore_fpr(17); restore_fpr(18); restore_fpr(19);
     restore_fpr(20); restore_fpr(21); restore_fpr(22); restore_fpr(23);
     restore_fpr(24); restore_fpr(25); restore_fpr(26); restore_fpr(27);
     restore_fpr(28); restore_fpr(29); restore_fpr(30); restore_fpr(31);

     err |= __get_user(current->thread.fpu.soft.sr, &sc->sc_fpc_csr);

     return err;
}

static inline int save_thread_fp_context(struct sigcontext *sc)
{
     u64 *pfreg = &current->thread.fpu.soft.regs[0];
     int err = 0;

#define save_fpr(i)                                     \
     do { err |= __put_user(pfreg[i], &sc->sc_fpregs[i]); } while(0)

     save_fpr( 0); save_fpr( 1); save_fpr( 2); save_fpr( 3);
     save_fpr( 4); save_fpr( 5); save_fpr( 6); save_fpr( 7);
     save_fpr( 8); save_fpr( 9); save_fpr(10); save_fpr(11);
     save_fpr(12); save_fpr(13); save_fpr(14); save_fpr(15);
     save_fpr(16); save_fpr(17); save_fpr(18); save_fpr(19);
     save_fpr(20); save_fpr(21); save_fpr(22); save_fpr(23);
     save_fpr(24); save_fpr(25); save_fpr(26); save_fpr(27);
     save_fpr(28); save_fpr(29); save_fpr(30); save_fpr(31);

     err |= __put_user(current->thread.fpu.soft.sr, &sc->sc_fpc_csr);

     return err;
}

asmlinkage int restore_sigcontext(struct pt_regs *regs, struct sigcontext *sc)
{
	int owned_fp;
	int err = 0;

	err |= __get_user(regs->cp0_epc, &sc->sc_pc);
	err |= __get_user(regs->hi, &sc->sc_mdhi);
	err |= __get_user(regs->lo, &sc->sc_mdlo);

#define restore_gp_reg(i) do {						\
	err |= __get_user(regs->regs[i], &sc->sc_regs[i]);		\
} while(0)
	restore_gp_reg( 1); restore_gp_reg( 2); restore_gp_reg( 3);
	restore_gp_reg( 4); restore_gp_reg( 5); restore_gp_reg( 6);
	restore_gp_reg( 7); restore_gp_reg( 8); restore_gp_reg( 9);
	restore_gp_reg(10); restore_gp_reg(11); restore_gp_reg(12);
	restore_gp_reg(13); restore_gp_reg(14); restore_gp_reg(15);
	restore_gp_reg(16); restore_gp_reg(17); restore_gp_reg(18);
	restore_gp_reg(19); restore_gp_reg(20); restore_gp_reg(21);
	restore_gp_reg(22); restore_gp_reg(23); restore_gp_reg(24);
	restore_gp_reg(25); restore_gp_reg(26); restore_gp_reg(27);
	restore_gp_reg(28); restore_gp_reg(29); restore_gp_reg(30);
	restore_gp_reg(31);
#undef restore_gp_reg

	err |= __get_user(owned_fp, &sc->sc_ownedfp);
	err |= __get_user(current->used_math, &sc->sc_used_math);

	if (owned_fp) {
		err |= restore_fp_context(sc);
		goto out;
	}

	if (current == last_task_used_math) {
		/* Signal handler acquired FPU - give it back */
		last_task_used_math = NULL;
		regs->cp0_status &= ~ST0_CU1;
	}

	if (current->used_math) {
		/* Undo possible contamination of thread state */
		err |= restore_thread_fp_context(sc);
	}

out:
	return err;
}

struct sigframe {
	u32 sf_ass[4];			/* argument save space for o32 */
	u32 sf_code[2];			/* signal trampoline */
	struct sigcontext sf_sc;
	sigset_t sf_mask;
};

struct rt_sigframe {
	u32 rs_ass[4];			/* argument save space for o32 */
	u32 rs_code[2];			/* signal trampoline */
	struct siginfo rs_info;
	struct ucontext rs_uc;
};

static int inline setup_sigcontext(struct pt_regs *regs, struct sigcontext *sc)
{
	int owned_fp;
	int err = 0;

	err |= __put_user(regs->cp0_epc, &sc->sc_pc);
	err |= __put_user(regs->cp0_status, &sc->sc_status);

#define save_gp_reg(i) do {						\
	err |= __put_user(regs->regs[i], &sc->sc_regs[i]);		\
} while(0)
	__put_user(0, &sc->sc_regs[0]); save_gp_reg(1); save_gp_reg(2);
	save_gp_reg(3); save_gp_reg(4); save_gp_reg(5); save_gp_reg(6);
	save_gp_reg(7); save_gp_reg(8); save_gp_reg(9); save_gp_reg(10);
	save_gp_reg(11); save_gp_reg(12); save_gp_reg(13); save_gp_reg(14);
	save_gp_reg(15); save_gp_reg(16); save_gp_reg(17); save_gp_reg(18);
	save_gp_reg(19); save_gp_reg(20); save_gp_reg(21); save_gp_reg(22);
	save_gp_reg(23); save_gp_reg(24); save_gp_reg(25); save_gp_reg(26);
	save_gp_reg(27); save_gp_reg(28); save_gp_reg(29); save_gp_reg(30);
	save_gp_reg(31);
#undef save_gp_reg

	err |= __put_user(regs->hi, &sc->sc_mdhi);
	err |= __put_user(regs->lo, &sc->sc_mdlo);
	err |= __put_user(regs->cp0_cause, &sc->sc_cause);
	err |= __put_user(regs->cp0_badvaddr, &sc->sc_badvaddr);

	owned_fp = (current == last_task_used_math);
	err |= __put_user(owned_fp, &sc->sc_ownedfp);
	err |= __put_user(current->used_math, &sc->sc_used_math);

	if (!current->used_math)
		goto out;

	/* There exists FP thread state that may be trashed by signal */
	if (owned_fp) {
		/* fp is active.  Save context from FPU */
		err |= save_fp_context(sc);
		goto out;
	}

	/*
	 * Someone else has FPU.
	 * Copy Thread context into signal context
	 */
	err |= save_thread_fp_context(sc);

out:
	return err;
}

/*
 * Determine which stack to use..
 */
static inline void *get_sigframe(struct k_sigaction *ka, struct pt_regs *regs,
	size_t frame_size)
{
	unsigned long sp;

	/* Default to using normal stack */
	sp = regs->regs[29];

	/*
 	 * FPU emulator may have it's own trampoline active just
 	 * above the user stack, 16-bytes before the next lowest
 	 * 16 byte boundary.  Try to avoid trashing it.
 	 */
 	sp -= 32;

	/* This is the X/Open sanctioned signal stack switching.  */
	if ((ka->sa.sa_flags & SA_ONSTACK) && ! on_sig_stack(sp))
		sp = current->sas_ss_sp + current->sas_ss_size;

	return (void *)((sp - frame_size) & ALMASK);
}

static void inline setup_frame(struct k_sigaction * ka, struct pt_regs *regs,
	int signr, sigset_t *set)
{
	struct sigframe *frame;
	int err = 0;

	frame = get_sigframe(ka, regs, sizeof(*frame));
	if (!access_ok(VERIFY_WRITE, frame, sizeof (*frame)))
		goto give_sigsegv;

	/*
	 * Set up to return from userspace.  On mips64 we always use a stub 
	 * already provided by userspace and ignore SA_RESTORER.
	 */
	regs->regs[31] = (unsigned long) ka->sa.sa_restorer;

	err |= setup_sigcontext(regs, &frame->sf_sc);
	err |= __copy_to_user(&frame->sf_mask, set, sizeof(*set));
	if (err)
		goto give_sigsegv;

	/*
	 * Arguments to signal handler:
	 *
	 *   a0 = signal number
	 *   a1 = 0 (should be cause)
	 *   a2 = pointer to struct sigcontext
	 *
	 * $25 and c0_epc point to the signal handler, $29 points to the
	 * struct sigframe.
	 */
	regs->regs[ 4] = signr;
	regs->regs[ 5] = 0;
	regs->regs[ 6] = (unsigned long) &frame->sf_sc;
	regs->regs[29] = (unsigned long) frame;
	regs->regs[31] = (unsigned long) frame->sf_code;
	regs->cp0_epc = regs->regs[25] = (unsigned long) ka->sa.sa_handler;

#if DEBUG_SIG
	printk("SIG deliver (%s:%d): sp=0x%p pc=0x%lx ra=0x%p\n",
	       current->comm, current->pid,
	       frame, regs->cp0_epc, frame->sf_code);
#endif
        return;

give_sigsegv:
	if (signr == SIGSEGV)
		ka->sa.sa_handler = SIG_DFL;
	force_sig(SIGSEGV, current);
}

static void inline setup_rt_frame(struct k_sigaction * ka,
	struct pt_regs *regs, int signr, sigset_t *set, siginfo_t *info)
{
	struct rt_sigframe *frame;
	int err = 0;

	frame = get_sigframe(ka, regs, sizeof(*frame));
	if (!access_ok(VERIFY_WRITE, frame, sizeof (*frame)))
		goto give_sigsegv;

	/*
	 * Set up to return from userspace.  On mips64 we always use a stub 
	 * already provided by userspace and ignore SA_RESTORER.
	 */
	regs->regs[31] = (unsigned long) ka->sa.sa_restorer;

	/* Create siginfo.  */
	err |= copy_siginfo_to_user(&frame->rs_info, info);

	/* Create the ucontext.  */
	err |= __put_user(0, &frame->rs_uc.uc_flags);
	err |= __put_user(0, &frame->rs_uc.uc_link);
	err |= __put_user((void *)current->sas_ss_sp,
	                  &frame->rs_uc.uc_stack.ss_sp);
	err |= __put_user(sas_ss_flags(regs->regs[29]),
	                  &frame->rs_uc.uc_stack.ss_flags);
	err |= __put_user(current->sas_ss_size,
	                  &frame->rs_uc.uc_stack.ss_size);
	err |= setup_sigcontext(regs, &frame->rs_uc.uc_mcontext);
	err |= __copy_to_user(&frame->rs_uc.uc_sigmask, set, sizeof(*set));

	if (err)
		goto give_sigsegv;

	/*
	 * Arguments to signal handler:
	 *
	 *   a0 = signal number
	 *   a1 = 0 (should be cause)
	 *   a2 = pointer to ucontext
	 *
	 * $25 and c0_epc point to the signal handler, $29 points to
	 * the struct rt_sigframe.
	 */
	regs->regs[ 4] = signr;
	regs->regs[ 5] = (unsigned long) &frame->rs_info;
	regs->regs[ 6] = (unsigned long) &frame->rs_uc;
	regs->regs[29] = (unsigned long) frame;
	regs->regs[31] = (unsigned long) frame->rs_code;
	regs->cp0_epc = regs->regs[25] = (unsigned long) ka->sa.sa_handler;

#if DEBUG_SIG
	printk("SIG deliver (%s:%d): sp=0x%p pc=0x%lx ra=0x%p\n",
	       current->comm, current->pid,
	       frame, regs->cp0_epc, frame->rs_code);
#endif
	return;

give_sigsegv:
	if (signr == SIGSEGV)
		ka->sa.sa_handler = SIG_DFL;
	force_sig(SIGSEGV, current);
}

static inline void handle_signal(unsigned long sig, siginfo_t *info,
	sigset_t *oldset, struct pt_regs *regs)
{
	struct k_sigaction *ka = &current->sig->action[sig-1];

	switch(regs->regs[0]) {
	case ERESTARTNOHAND:
		regs->regs[2] = EINTR;
		break;
	case ERESTARTSYS:
		if(!(ka->sa.sa_flags & SA_RESTART)) {
			regs->regs[2] = EINTR;
			break;
		}
	/* fallthrough */
	case ERESTARTNOINTR:		/* Userland will reload $v0.  */
		regs->regs[7] = regs->regs[26];
		regs->cp0_epc -= 8;
	}

	regs->regs[0] = 0;		/* Don't deal with this again.  */

	if (ka->sa.sa_flags & SA_SIGINFO)
		setup_rt_frame(ka, regs, sig, oldset, info);
	else
		setup_frame(ka, regs, sig, oldset);

	if (ka->sa.sa_flags & SA_ONESHOT)
		ka->sa.sa_handler = SIG_DFL;
	if (!(ka->sa.sa_flags & SA_NODEFER)) {
		spin_lock_irq(&current->sigmask_lock);
		sigorsets(&current->blocked,&current->blocked,&ka->sa.sa_mask);
		sigaddset(&current->blocked,sig);
		recalc_sigpending();
		spin_unlock_irq(&current->sigmask_lock);
	}
}

asmlinkage int do_signal(sigset_t *oldset, struct pt_regs *regs)
{
	siginfo_t info;
	int signr;

	if (!oldset)
		oldset = &current->blocked;

	signr = get_signal_to_deliver(&info, regs);
	if (signr > 0) {
		handle_signal(signr, &info, oldset, regs);
		return 1;
	}

	/*
	 * Who's code doesn't conform to the restartable syscall convention
	 * dies here!!!  The li instruction, a single machine instruction,
	 * must directly be followed by the syscall instruction.
	 */
	if (regs->regs[0]) {
		if (regs->regs[2] == ERESTARTNOHAND ||
		    regs->regs[2] == ERESTARTSYS ||
		    regs->regs[2] == ERESTARTNOINTR) {
			regs->regs[7] = regs->regs[26];
			regs->cp0_epc -= 8;
		}
	}
	return 0;
}

extern int do_irix_signal(sigset_t *oldset, struct pt_regs *regs);
extern int do_signal32(sigset_t *oldset, struct pt_regs *regs);

/*
 * notification of userspace execution resumption
 * - triggered by current->work.notify_resume
 */
asmlinkage void do_notify_resume(struct pt_regs *regs, sigset_t *oldset,
	__u32 thread_info_flags)
{
	/* deal with pending signal delivery */
	if (thread_info_flags & _TIF_SIGPENDING) {
#ifdef CONFIG_BINFMT_ELF32
		if (likely((current->thread.mflags & MF_32BIT))) {
			do_signal32(oldset, regs);
			return;
		}
#endif
#ifdef CONFIG_BINFMT_IRIX
		if (unlikely(current->personality != PER_LINUX)) {
			do_irix_signal(oldset, regs);
			return;
		}
#endif
		do_signal(regs,oldset);
	}
}
