/*
 *  linux/arch/mips/kernel/signal.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *  Copyright (C) 1994, 1995, 1996  Ralf Baechle
 */
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>

#include <asm/asm.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/cache.h>
#include <asm/mipsconfig.h>
#include <asm/sgidefs.h>

/*
 * Linux/MIPS misstreats the SA_NOMASK flag for signal handlers.
 * Actually this is a bug in libc that was made visible by the POSIX.1
 * changes in Linux/MIPS 2.0.1.  To keep old binaries alive enable
 * this define but note that this is just a hack with sideeffects, not a
 * perfect compatibility mode.  This will go away, so rebuild your
 * executables with libc 960709 or newer.
 */
#define CONF_NOMASK_BUG_COMPAT

#define _S(nr) (1<<((nr)-1))

#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

asmlinkage int sys_waitpid(pid_t pid,unsigned long * stat_addr, int options);
asmlinkage int do_signal(unsigned long oldmask, struct pt_regs *regs);

asmlinkage void (*save_fp_context)(struct sigcontext *sc);
extern asmlinkage void (*restore_fp_context)(struct sigcontext *sc);

asmlinkage int sys_sigprocmask(int how, sigset_t *set, sigset_t *oset)
{
	k_sigset_t new_set, old_set = current->blocked;

	if (set) {
		if (!access_ok(VERIFY_READ, set, sizeof(sigset_t)))
			return -EFAULT;

		__get_user(new_set, to_k_sigset_t(set));
		new_set &= _BLOCKABLE;

		switch (how) {
		case SIG_BLOCK:
			current->blocked |= new_set;
			break;
		case SIG_UNBLOCK:
			current->blocked &= ~new_set;
			break;
		case SIG_SETMASK:
			current->blocked = new_set;
			break;
		/*
		 * SGI goodie: Just set the low 32 bits of 'blocked' even
		 * for 128 bit sigset_t.
		 */
		case SIG_SETMASK32:
			current->blocked = new_set;
			break;
		default:
			return -EINVAL;
		}
	}
	if (oset) {
		if(!access_ok(VERIFY_WRITE, oset, sizeof(sigset_t)))
			return -EFAULT;
		__put_user(old_set, &oset->__sigbits[0]);
		__put_user(0, &oset->__sigbits[1]);
		__put_user(0, &oset->__sigbits[2]);
		__put_user(0, &oset->__sigbits[3]);
	}

	return 0;
}

/*
 * Atomically swap in the new signal mask, and wait for a signal.
 */
asmlinkage int sys_sigsuspend(struct pt_regs *regs)
{
	unsigned int mask;
	sigset_t *uset;
	k_sigset_t kset;

	mask = current->blocked;
	uset = (sigset_t *)(long) regs->regs[4];
	if (!access_ok(VERIFY_READ, uset, sizeof(sigset_t)))
		return -EFAULT;

	__get_user(kset, to_k_sigset_t(uset));

	current->blocked = kset & _BLOCKABLE;
	regs->regs[2] = -EINTR;
	while (1) {
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		if (do_signal(mask, regs))
			return -EINTR;
	}

	return -EINTR;
}

asmlinkage int sys_sigreturn(struct pt_regs *regs)
{
	struct sigcontext *context;
	int i;

	/*
	 * We don't support fixing ADEL/ADES exceptions for signal stack frames.
	 * No big loss - who doesn't care about the alignment of this stack
	 * really deserves to loose.
	 */
	context = (struct sigcontext *)(long) regs->regs[29];
	if (!access_ok(VERIFY_READ, context, sizeof(struct sigcontext)) ||
	    (regs->regs[29] & (SZREG - 1)))
		goto badframe;

	current->blocked = context->sc_sigset.__sigbits[0] & _BLOCKABLE;
	regs->cp0_epc = context->sc_pc;
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
	for(i = 31;i >= 0;i--)
		__get_user(regs->regs[i], &context->sc_regs[i]);
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
	/*
	 * We only allow user processes in 64bit mode (n32, 64 bit ABI) to
	 * restore the upper half of registers.
	 */
	if (read_32bit_cp0_register(CP0_STATUS) & ST0_UX)
		for(i = 31;i >= 0;i--)
			__get_user(regs->regs[i], &context->sc_regs[i]);
	else
		for(i = 31;i >= 0;i--) {
			__get_user(regs->regs[i], &context->sc_regs[i]);
			regs->regs[i] = (int) regs->regs[i];
		}
#endif
	__get_user(regs->hi, &context->sc_mdhi);
	__get_user(regs->lo, &context->sc_mdlo);
	restore_fp_context(context);

	/*
	 * Disable syscall checks
	 */
	regs->orig_reg2 = -1;

	/*
	 * Don't let your children do this ...
	 */
	asm(	"move\t$29,%0\n\t"
		"j\tret_from_sys_call"
		:/* no outputs */
		:"r" (regs));
	/* Unreached */

badframe:
	do_exit(SIGSEGV);
}

/*
 * Set up a signal frame...
 *
 * This routine is somewhat complicated by the fact that if the kernel may be
 * entered by an exception other than a system call; e.g. a bus error or other
 * "bad" exception.  If this is the case, then *all* the context on the kernel
 * stack frame must be saved.
 *
 * For a large number of exceptions, the stack frame format is the same
 * as that which will be created when the process traps back to the kernel
 * when finished executing the signal handler.	In this case, nothing
 * must be done. This information is saved on the user stack and restored
 * when the signal handler is returned.
 *
 * The signal handler will be called with ra pointing to code1 (see below) and
 * signal number and pointer to the saved sigcontext as the two parameters.
 *
 *     usp ->  [unused]                         ; first free word on stack
 *             arg save space                   ; 16/32 bytes arg. save space
 *	       code1   (addiu sp,#1-offset)	; pop stackframe
 *	       code2   (li v0,__NR_sigreturn)	; syscall number
 *	       code3   (syscall)		; do sigreturn(2)
 *     #1|     $0, at, v0, v1, a0, a1, a2, a3   ; All integer registers
 *       |     t0, t1, t2, t3, t4, t5, t6, t7   ; $0, k0 and k1 are placeholders
 *       |     s0, s1, s2, s3, s4, s5, s6, s7
 *       |     k0, k1, t8, t9, gp, sp, fp, ra;
 *       |     epc                              ; old program counter
 *       |     cause                            ; CP0 cause register
 *       |     oldmask
 */

struct sc {
	unsigned long	ass[4];
	unsigned int	code[4];
	struct sigcontext scc;
};
#define scc_offset ((size_t)&((struct sc *)0)->scc)

static void setup_frame(struct sigaction * sa, struct pt_regs *regs,
                        int signr, unsigned long oldmask)
{
	struct sc *frame;
	struct sigcontext *sc;
	int i;

	frame = (struct sc *) (long) regs->regs[29];
	frame--;

	/*
	 * We realign the stack to an adequate boundary for the architecture.
         * The assignment to sc had to be moved over the if to prevent
         * GCC from throwing warnings.
	 */
	frame  = (struct sc *)((unsigned long)frame & ALMASK);
	sc = &frame->scc;
	if (!access_ok(VERIFY_WRITE, frame, sizeof (struct sc))) {
		do_exit(SIGSEGV);
		return;
	}

	/*
	 * Set up the return code ...
	 *
	 *         .set    noreorder
	 *         addiu   sp,24
	 *         li      v0,__NR_sigreturn
	 *         syscall
	 *         .set    reorder
	 */
	__put_user(0x27bd0000 + scc_offset,     &frame->code[0]);
	__put_user(0x24020000 + __NR_sigreturn, &frame->code[1]);
	__put_user(0x0000000c,                  &frame->code[2]);

	/*
	 * Flush caches so that the instructions will be correctly executed.
	 */
	cacheflush((unsigned long)frame->code, sizeof (frame->code),
                   CF_BCACHE|CF_ALL);

	/*
	 * Set up the "normal" sigcontext
	 */
	sc->sc_pc = regs->cp0_epc;			/* Program counter */
	sc->sc_status = regs->cp0_status;		/* Status register */
	for(i = 31;i >= 0;i--)
		__put_user(regs->regs[i], &sc->sc_regs[i]);
	save_fp_context(sc);
	__put_user(regs->hi, &sc->sc_mdhi);
	__put_user(regs->lo, &sc->sc_mdlo);
	__put_user(regs->cp0_cause, &sc->sc_cause);
	__put_user((regs->cp0_status & ST0_CU0) != 0, &sc->sc_ownedfp);
	__put_user(oldmask, &sc->sc_sigset.__sigbits[0]);
	__put_user(0, &sc->sc_sigset.__sigbits[1]);
	__put_user(0, &sc->sc_sigset.__sigbits[2]);
	__put_user(0, &sc->sc_sigset.__sigbits[3]);

	regs->regs[4] = signr;				/* Args for handler */
	regs->regs[5] = (long) frame;			/* Ptr to sigcontext */
	regs->regs[29] = (unsigned long) frame;		/* Stack pointer */
	regs->regs[31] = (unsigned long) frame->code;	/* Return address */
	regs->cp0_epc = regs->regs[25]			/* "return" to the first handler */
	              = (unsigned long) sa->sa_handler;
}

/*
 * OK, we're invoking a handler
 */	
static inline void
handle_signal(unsigned long signr, struct sigaction *sa,
              unsigned long oldmask, struct pt_regs * regs)
{
	/* are we from a failed system call? */
	if (regs->orig_reg2 >= 0 && regs->regs[7]) {
		/* If so, check system call restarting.. */
		switch (regs->regs[2]) {
			case ERESTARTNOHAND:
				regs->regs[2] = EINTR;
				break;

			case ERESTARTSYS:
				if (!(sa->sa_flags & SA_RESTART)) {
					regs->regs[2] = EINTR;
					break;
				}
			/* fallthrough */
			case ERESTARTNOINTR:
				regs->regs[7] = regs->orig_reg7;
				regs->cp0_epc -= 8;
		}
	}

	/* set up the stack frame */
	setup_frame(sa, regs, signr, oldmask);

	if (sa->sa_flags & SA_ONESHOT)
		sa->sa_handler = NULL;
#ifdef CONF_NOMASK_BUG_COMPAT
	current->blocked |= *to_k_sigset_t(&sa->sa_mask);
#else
	if (!(sa->sa_flags & SA_NOMASK))
		current->blocked |= (*to_k_sigset_t(&sa->sa_mask) |
		                     _S(signr)) & _BLOCKABLE;
#endif
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 *
 * Note that we go through the signals twice: once to check the signals that
 * the kernel can handle, and then we build all the user-level signal handling
 * stack-frames in one go after that.
 */
asmlinkage int do_signal(unsigned long oldmask, struct pt_regs * regs)
{
	unsigned long mask = ~current->blocked;
	unsigned long signr;
	struct sigaction * sa;

	while ((signr = current->signal & mask)) {
		signr = ffz(~signr);
		clear_bit(signr, &current->signal);
		sa = current->sig->action + signr;
		signr++;
		if ((current->flags & PF_PTRACED) && signr != SIGKILL) {
			current->exit_code = signr;
			current->state = TASK_STOPPED;
			notify_parent(current);
			schedule();
			if (!(signr = current->exit_code))
				continue;
			current->exit_code = 0;
			if (signr == SIGSTOP)
				continue;
			if (_S(signr) & current->blocked) {
				current->signal |= _S(signr);
				continue;
			}
			sa = current->sig->action + signr - 1;
		}
		if (sa->sa_handler == SIG_IGN) {
			if (signr != SIGCHLD)
				continue;
			/* check for SIGCHLD: it's special */
			while (sys_waitpid(-1,NULL,WNOHANG) > 0)
				/* nothing */;
			continue;
		}
		if (sa->sa_handler == SIG_DFL) {
			if (current->pid == 1)
				continue;
			switch (signr) {
			case SIGCONT: case SIGCHLD: case SIGWINCH:
				continue;

			case SIGTSTP: case SIGTTIN: case SIGTTOU:
				if (is_orphaned_pgrp(current->pgrp))
					continue;
			case SIGSTOP:
				if (current->flags & PF_PTRACED)
					continue;
				current->state = TASK_STOPPED;
				current->exit_code = signr;
				if (!(current->p_pptr->sig->action[SIGCHLD-1].sa_flags & 
						SA_NOCLDSTOP))
					notify_parent(current);
				schedule();
				continue;

			case SIGQUIT: case SIGILL: case SIGTRAP:
			case SIGABRT: case SIGFPE: case SIGSEGV:
			case SIGBUS:
				if (current->binfmt && current->binfmt->core_dump) {
					if (current->binfmt->core_dump(signr, regs))
						signr |= 0x80;
				}
				/* fall through */
			default:
				current->signal |= _S(signr & 0x7f);
				current->flags |= PF_SIGNALED;
				do_exit(signr);
			}
		}
		handle_signal(signr, sa, oldmask, regs);
		return 1;
	}

	/* Did we come from a system call? */
	if (regs->orig_reg2 >= 0) {
		/* Restart the system call - no handlers present */
		if (regs->regs[2] == -ERESTARTNOHAND ||
		    regs->regs[2] == -ERESTARTSYS ||
		    regs->regs[2] == -ERESTARTNOINTR) {
			regs->regs[2] = regs->orig_reg2;
			regs->cp0_epc -= 8;
		}
	}
	return 0;
}

/*
 * The signal(2) syscall is no longer available in the kernel
 * because GNU libc doesn't use it.  Maybe I'll add it back to the
 * kernel for the binary compatibility stuff.
 */
asmlinkage unsigned long sys_signal(int signum, __sighandler_t handler)
{
        return -ENOSYS;
}
