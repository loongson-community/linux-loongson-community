/*
 *  linux/kernel/signal.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>

#include <asm/segment.h>
#include <asm/cachectl.h>

#define _S(nr) (1<<((nr)-1))

#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

asmlinkage int do_signal(unsigned long oldmask, struct pt_regs * regs);

asmlinkage int sys_sigprocmask(int how, sigset_t *set, sigset_t *oset)
{
	sigset_t new_set, old_set = current->blocked;
	int error;

	if (set) {
		error = verify_area(VERIFY_READ, set, sizeof(sigset_t));
		if (error)
			return error;
		new_set = get_fs_long((unsigned long *) set) & _BLOCKABLE;
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
		default:
			return -EINVAL;
		}
	}
	if (oset) {
		error = verify_area(VERIFY_WRITE, oset, sizeof(sigset_t));
		if (error)
			return error;
		put_fs_long(old_set, (unsigned long *) oset);
	}
	return 0;
}

asmlinkage int sys_sgetmask(void)
{
	return current->blocked;
}

asmlinkage int sys_ssetmask(int newmask)
{
	int old=current->blocked;

	current->blocked = newmask & _BLOCKABLE;
	return old;
}

asmlinkage int sys_sigpending(sigset_t *set)
{
	int error;
	/* fill in "set" with signals pending but blocked. */
	error = verify_area(VERIFY_WRITE, set, 4);
	if (!error)
		put_fs_long(current->blocked & current->signal, (unsigned long *)set);
	return error;
}

/*
 * atomically swap in the new signal mask, and wait for a signal.
 */
asmlinkage int sys_sigsuspend(int restart, unsigned long oldmask, unsigned long set)
{
	unsigned long mask;
	struct pt_regs * regs = (struct pt_regs *) &restart;

	mask = current->blocked;
	current->blocked = set & _BLOCKABLE;
#if defined (__i386__)
	regs->eax = -EINTR;
#elif defined (__mips__)
	regs->reg2 = -EINTR;
#endif
	while (1) {
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		if (do_signal(mask,regs))
			return -EINTR;
	}
}

/*
 * POSIX 3.3.1.3:
 *  "Setting a signal action to SIG_IGN for a signal that is pending
 *   shall cause the pending signal to be discarded, whether or not
 *   it is blocked" (but SIGCHLD is unspecified: linux leaves it alone).
 *
 *  "Setting a signal action to SIG_DFL for a signal that is pending
 *   and whose default action is to ignore the signal (for example,
 *   SIGCHLD), shall cause the pending signal to be discarded, whether
 *   or not it is blocked"
 *
 * Note the silly behaviour of SIGCHLD: SIG_IGN means that the signal
 * isn't actually ignored, but does automatic child reaping, while
 * SIG_DFL is explicitly said by POSIX to force the signal to be ignored..
 */
static void check_pending(int signum)
{
	struct sigaction *p;

	p = signum - 1 + current->sigaction;
	if (p->sa_handler == SIG_IGN) {
		if (signum == SIGCHLD)
			return;
		current->signal &= ~_S(signum);
		return;
	}
	if (p->sa_handler == SIG_DFL) {
		if (signum != SIGCONT && signum != SIGCHLD && signum != SIGWINCH)
			return;
		current->signal &= ~_S(signum);
		return;
	}	
}

asmlinkage int sys_signal(int signum, unsigned long handler)
{
	struct sigaction tmp;

	if (signum<1 || signum>32)
		return -EINVAL;
	if (signum==SIGKILL || signum==SIGSTOP)
		return -EINVAL;
	if (handler >= TASK_SIZE)
		return -EFAULT;
	tmp.sa_handler = (void (*)(int)) handler;
	tmp.sa_mask = 0;
	tmp.sa_flags = SA_ONESHOT | SA_NOMASK;
	tmp.sa_restorer = NULL;
	handler = (long) current->sigaction[signum-1].sa_handler;
	current->sigaction[signum-1] = tmp;
	check_pending(signum);
	return handler;
}

asmlinkage int sys_sigaction(int signum, const struct sigaction * action,
	struct sigaction * oldaction)
{
	struct sigaction new_sa, *p;

	if (signum<1 || signum>32)
		return -EINVAL;
	if (signum==SIGKILL || signum==SIGSTOP)
		return -EINVAL;
	p = signum - 1 + current->sigaction;
	if (action) {
		int err = verify_area(VERIFY_READ, action, sizeof(*action));
		if (err)
			return err;
		memcpy_fromfs(&new_sa, action, sizeof(struct sigaction));
		if (new_sa.sa_flags & SA_NOMASK)
			new_sa.sa_mask = 0;
		else {
			new_sa.sa_mask |= _S(signum);
			new_sa.sa_mask &= _BLOCKABLE;
		}
		if (TASK_SIZE <= (unsigned long) new_sa.sa_handler)
			return -EFAULT;
	}
	if (oldaction) {
		int err = verify_area(VERIFY_WRITE, oldaction, sizeof(*oldaction));
		if (err)
			return err;
		memcpy_tofs(oldaction, p, sizeof(struct sigaction));
	}
	if (action) {
		*p = new_sa;
		check_pending(signum);
	}
	return 0;
}

asmlinkage int sys_waitpid(pid_t pid,unsigned long * stat_addr, int options);

/*
 * This sets regs->reg29 even though we don't actually use sigstacks yet..
 */
asmlinkage int sys_sigreturn(unsigned long __unused)
{
	struct sigcontext_struct context;
	struct pt_regs * regs;

	regs = (struct pt_regs *) &__unused;
	if (verify_area(VERIFY_READ, (void *) regs->reg29, sizeof(context)))
		goto badframe;
	memcpy_fromfs(&context,(void *) regs->reg29, sizeof(context));
	current->blocked = context.oldmask & _BLOCKABLE;
	regs->reg1  = context.sc_at;
	regs->reg2  = context.sc_v0;
	regs->reg3  = context.sc_v1;
	regs->reg4  = context.sc_a0;
	regs->reg5  = context.sc_a1;
	regs->reg6  = context.sc_a2;
	regs->reg7  = context.sc_a3;
	regs->reg8  = context.sc_t0;
	regs->reg9  = context.sc_t1;
	regs->reg10 = context.sc_t2;
	regs->reg11 = context.sc_t3;
	regs->reg12 = context.sc_t4;
	regs->reg13 = context.sc_t5;
	regs->reg14 = context.sc_t6;
	regs->reg15 = context.sc_t7;
	regs->reg24 = context.sc_t8;
	regs->reg25 = context.sc_t9;
	regs->reg29 = context.sc_sp;

	/*
	 * disable syscall checks
	 */
	regs->orig_reg2 = -1;
	return regs->orig_reg2;
badframe:
	do_exit(SIGSEGV);
}

/*
 * Set up a signal frame...
 */
static void setup_frame(struct sigaction * sa, unsigned long ** fp,
                        unsigned long pc, struct pt_regs *regs,
                        int signr, unsigned long oldmask)
{
	unsigned long * frame;

	frame = *fp;
	frame -= 21;
	if (verify_area(VERIFY_WRITE,frame,21*4))
		do_exit(SIGSEGV);
	/*
	 * set up the "normal" stack seen by the signal handler (iBCS2)
	 */
	put_fs_long(regs->reg1 , frame   );
	put_fs_long(regs->reg2 , frame+ 1);
	put_fs_long(regs->reg3 , frame+ 2);
	put_fs_long(regs->reg4 , frame+ 3);
	put_fs_long(regs->reg5 , frame+ 4);
	put_fs_long(regs->reg6 , frame+ 5);
	put_fs_long(regs->reg7 , frame+ 6);
	put_fs_long(regs->reg8 , frame+ 7);
	put_fs_long(regs->reg10, frame+ 8);
	put_fs_long(regs->reg11, frame+ 9);
	put_fs_long(regs->reg12, frame+10);
	put_fs_long(regs->reg13, frame+11);
	put_fs_long(regs->reg14, frame+12);
	put_fs_long(regs->reg15, frame+13);
	put_fs_long(regs->reg16, frame+14);
	put_fs_long(regs->reg17, frame+15);
	put_fs_long(regs->reg24, frame+16);
	put_fs_long(regs->reg25, frame+17);
	put_fs_long(regs->reg29, frame+18);
	put_fs_long(pc         , frame+19);
	put_fs_long(oldmask    , frame+20);
	/*
	 * set up the return code...
	 *
	 *         .set    noat
	 *         syscall
	 *         li      $1,__NR_sigreturn
	 *         .set    at
	 */
	put_fs_long(0x24010077, frame+20);		/* li	$1,119 */
	put_fs_long(0x000000c0, frame+21);		/* syscall     */
	*fp = frame;
	/*
	 * Flush caches so the instructions will be correctly executed.
	 */
	cacheflush(frame, 21*4, BCACHE);
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
	unsigned long handler_signal = 0;
	unsigned long *frame = NULL;
	unsigned long pc = 0;
	unsigned long signr;
	struct sigaction * sa;

	while ((signr = current->signal & mask)) {
#if defined (__i386__)
		__asm__("bsf %2,%1\n\t"
			"btrl %1,%0"
			:"=m" (current->signal),"=r" (signr)
			:"1" (signr));
#elif defined (__mips__)
		__asm__(".set\tnoreorder\n\t"
			".set\tnoat\n\t"
			"li\t%1,1\n"
			"1:\tand\t$1,%2,%1\n\t"
			"beq\t$0,$1,2f\n\t"
			"sll\t%2,%2,1\n\t"
			"bne\t$0,%2,1b\n\t"
			"add\t%0,%0,1\n"
			"2:\tli\t%2,-2\n\t"
			"sllv\t%2,%2,%0\n\t"
			"and\t%1,%1,%2\n\t"
			".set\tat\n\t"
			".set\treorder\n"
			"2:\n\t"
			:"=r" (signr),"=r" (current->signal),"=r" (mask)
			:"0"  (signr),"1"  (current->signal)
			:"$1");
#endif
		sa = current->sigaction + signr;
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
			sa = current->sigaction + signr - 1;
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

			case SIGSTOP: case SIGTSTP: case SIGTTIN: case SIGTTOU:
				if (current->flags & PF_PTRACED)
					continue;
				current->state = TASK_STOPPED;
				current->exit_code = signr;
				if (!(current->p_pptr->sigaction[SIGCHLD-1].sa_flags & 
						SA_NOCLDSTOP))
					notify_parent(current);
				schedule();
				continue;

			case SIGQUIT: case SIGILL: case SIGTRAP:
			case SIGIOT: case SIGFPE: case SIGSEGV:
				if (current->binfmt && current->binfmt->core_dump) {
					if (current->binfmt->core_dump(signr, regs))
						signr |= 0x80;
				}
				/* fall through */
			default:
				current->signal |= _S(signr & 0x7f);
				do_exit(signr);
			}
		}
		/*
		 * OK, we're invoking a handler
		 */
		if (regs->orig_reg2 >= 0) {
			if (regs->reg2 == -ERESTARTNOHAND ||
			   (regs->reg2 == -ERESTARTSYS &&
			    !(sa->sa_flags & SA_RESTART)))
				regs->reg2 = -EINTR;
		}
		handler_signal |= 1 << (signr-1);
		mask &= ~sa->sa_mask;
	}
	if (regs->orig_reg2 >= 0 &&
	    (regs->reg2 == -ERESTARTNOHAND ||
	     regs->reg2 == -ERESTARTSYS ||
	     regs->reg2 == -ERESTARTNOINTR)) {
		regs->reg2 = regs->orig_reg2;
		regs->cp0_epc -= 2;
	}
	if (!handler_signal)		/* no handler will be called - return 0 */
		return 0;
	pc = regs->cp0_epc;
	frame = (unsigned long *) regs->reg29;
	signr = 1;
	sa = current->sigaction;
	for (mask = 1 ; mask ; sa++,signr++,mask += mask) {
		if (mask > handler_signal)
			break;
		if (!(mask & handler_signal))
			continue;
		setup_frame(sa,&frame,pc,regs,signr,oldmask);
		pc = (unsigned long) sa->sa_handler;
		if (sa->sa_flags & SA_ONESHOT)
			sa->sa_handler = NULL;
		/*
		 * force a kernel-mode page-in of the signal
		 * handler to reduce races
		 */
		__asm__(".set\tnoat\n\t"
			"lwu\t$1,(%0)\n\t"
			".set\tat\n\t"
			:
			:"r" ((char *) pc)
			:"$1");
		current->blocked |= sa->sa_mask;
		oldmask |= sa->sa_mask;
	}
	regs->reg29 = (unsigned long) frame;
	regs->cp0_epc = pc;		/* "return" to the first handler */
	return 1;
}
