/* $Id: irixsig.c,v 1.5 1996/08/05 09:20:56 dm Exp $
 * irixsig.c: WHEEE, IRIX signals!  YOW, am I compatable or what?!?!
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/time.h>

#include <asm/ptrace.h>
#include <asm/uaccess.h>

#undef DEBUG_SIG

#define _S(nr) (1<<((nr)-1))

#define _BLOCKABLE (~(_S(SIGKILL) | _S(SIGSTOP)))

struct sigctx_irix5 {
	u32 rmask, cp0_status;
	u64 pc;
	u64 regs[32];
	u64 fpregs[32];
	u32 usedfp, fpcsr, fpeir, sstk_flags;
	u64 hi, lo;
	u64 cp0_cause, cp0_badvaddr, _unused0;
	u32 sigset[4];
	u64 weird_fpu_thing;
	u64 _unused1[31];
};

#ifdef DEBUG_SIG
/* Debugging */
static inline void dump_irix5_sigctx(struct sigctx_irix5 *c)
{
	int i;

	printk("misc: rmask[%08lx] status[%08lx] pc[%08lx]\n",
	       (unsigned long) c->rmask,
	       (unsigned long) c->cp0_status,
	       (unsigned long) c->pc);
	printk("regs: ");
	for(i = 0; i < 16; i++)
		printk("[%d]<%08lx> ", i, (unsigned long) c->regs[i]);
	printk("\nregs: ");
	for(i = 16; i < 32; i++)
		printk("[%d]<%08lx> ", i, (unsigned long) c->regs[i]);
	printk("\nfpregs: ");
	for(i = 0; i < 16; i++)
		printk("[%d]<%08lx> ", i, (unsigned long) c->fpregs[i]);
	printk("\nfpregs: ");
	for(i = 16; i < 32; i++)
		printk("[%d]<%08lx> ", i, (unsigned long) c->fpregs[i]);
	printk("misc: usedfp[%d] fpcsr[%08lx] fpeir[%08lx] stk_flgs[%08lx]\n",
	       (int) c->usedfp, (unsigned long) c->fpcsr,
	       (unsigned long) c->fpeir, (unsigned long) c->sstk_flags);
	printk("misc: hi[%08lx] lo[%08lx] cause[%08lx] badvaddr[%08lx]\n",
	       (unsigned long) c->hi, (unsigned long) c->lo,
	       (unsigned long) c->cp0_cause, (unsigned long) c->cp0_badvaddr);
	printk("misc: sigset<0>[%08lx] sigset<1>[%08lx] sigset<2>[%08lx] "
	       "sigset<3>[%08lx]\n", (unsigned long) c->sigset[0],
	       (unsigned long) c->sigset[1], (unsigned long) c->sigset[2],
	       (unsigned long) c->sigset[3]);
}
#endif

static void setup_irix_frame(struct sigaction * sa, struct pt_regs *regs,
			     int signr, unsigned long oldmask)
{
	unsigned long sp;
	struct sigctx_irix5 *ctx;
	int i;

	sp = regs->regs[29];
	sp -= sizeof(struct sigctx_irix5);
	sp &= ~(0xf);
	ctx = (struct sigctx_irix5 *) sp;
	if (!access_ok(VERIFY_WRITE, ctx, sizeof(*ctx)))
		do_exit(SIGSEGV);

	__put_user(0, &ctx->weird_fpu_thing);
	__put_user(~(0x00000001), &ctx->rmask);
	__put_user(0, &ctx->regs[0]);
	for(i = 1; i < 32; i++)
		__put_user((u64) regs->regs[i], &ctx->regs[i]);

	__put_user((u64) regs->hi, &ctx->hi);
	__put_user((u64) regs->lo, &ctx->lo);
	__put_user((u64) regs->cp0_epc, &ctx->pc);
	__put_user(current->used_math, &ctx->usedfp);
	__put_user((u64) regs->cp0_cause, &ctx->cp0_cause);
	__put_user((u64) regs->cp0_badvaddr, &ctx->cp0_badvaddr);

	__put_user(0, &ctx->sstk_flags); /* XXX sigstack unimp... todo... */

	__put_user(0, &ctx->sigset[1]);
	__put_user(0, &ctx->sigset[2]);
	__put_user(0, &ctx->sigset[3]);
	__put_user(oldmask, &ctx->sigset[0]);

#ifdef DEBUG_SIG
	dump_irix5_sigctx(ctx);
#endif

	regs->regs[5] = 0; /* XXX sigcode XXX */
	regs->regs[4] = (unsigned long) signr;
	regs->regs[6] = regs->regs[29] = sp;
	regs->regs[7] = (unsigned long) sa->sa_handler;
	regs->regs[25] = regs->cp0_epc = current->tss.irix_trampoline;
}

extern asmlinkage int sys_waitpid(pid_t pid,unsigned long * stat_addr, int options);

asmlinkage int do_irix_signal(unsigned long oldmask, struct pt_regs * regs)
{
	unsigned long mask = ~current->blocked;
	unsigned long handler_signal = 0;
	unsigned long signr;
	struct sigaction * sa;

#ifdef DEBUG_SIG
	printk("[%s:%d] Delivering IRIX signal oldmask=%08lx\n",
	       current->comm, current->pid, oldmask);
#endif
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

			case SIGSTOP: case SIGTSTP: case SIGTTIN: case SIGTTOU:
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
			case SIGIOT: case SIGFPE: case SIGSEGV: case SIGBUS:
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
		/*
		 * OK, we're invoking a handler
		 */
		if (regs->orig_reg2 >= 0) {
			if (regs->regs[2] == ERESTARTNOHAND) {
				regs->regs[2] = EINTR;
			} else if((regs->regs[2] == ERESTARTSYS &&
				   !(sa->sa_flags & SA_RESTART))) {
				regs->regs[2] = regs->orig_reg2;
				regs->cp0_epc -= 8;
			}
		}
		handler_signal |= 1 << (signr-1);
		mask &= ~*to_k_sigset_t(&sa->sa_mask);
	}
	/*
	 * Who's code doesn't conform to the restartable syscall convention
	 * dies here!!!  The li instruction, a single machine instruction,
	 * must directly be followed by the syscall instruction.
	 */
	if (regs->orig_reg2 >= 0 &&
	    (regs->regs[2] == ERESTARTNOHAND ||
	     regs->regs[2] == ERESTARTSYS ||
	     regs->regs[2] == ERESTARTNOINTR)) {
		regs->regs[2] = regs->orig_reg2;
		regs->cp0_epc -= 8;
	}
	if (!handler_signal)		/* no handler will be called - return 0 */
		return 0;
	signr = 1;
	sa = current->sig->action;
	for (mask = 1 ; mask ; sa++,signr++,mask += mask) {
		if (mask > handler_signal)
			break;
		if (!(mask & handler_signal))
			continue;
		setup_irix_frame(sa, regs, signr, oldmask);
		if (sa->sa_flags & SA_ONESHOT)
			sa->sa_handler = NULL;
		current->blocked |= *to_k_sigset_t(&sa->sa_mask);
		oldmask |= *to_k_sigset_t(&sa->sa_mask);
	}

	return 1;
}

asmlinkage unsigned long irix_sigreturn(struct pt_regs *regs)
{
	struct sigctx_irix5 *context, *magic;
	unsigned long umask, mask;
	u64 *fregs, res;
	int sig, i, base = 0;

	if(regs->regs[2] == 1000)
		base = 1;

	context = (struct sigctx_irix5 *) regs->regs[base + 4];
	magic = (struct sigctx_irix5 *) regs->regs[base + 5];
	sig = (int) regs->regs[base + 6];
#ifdef DEBUG_SIG
	printk("[%s:%d] IRIX sigreturn(scp[%p],ucp[%p],sig[%d])\n",
	       current->comm, current->pid, context, magic, sig);
#endif
	if (!context)
		context = magic;
	if (!access_ok(VERIFY_READ, context, sizeof(struct sigctx_irix5)))
		goto badframe;

#ifdef DEBUG_SIG
	dump_irix5_sigctx(context);
#endif

	__get_user(regs->cp0_epc, &context->pc);
	umask = context->rmask; mask = 2;
	for (i = 1; i < 32; i++, mask <<= 1) {
		if(umask & mask)
			__get_user(regs->regs[i], &context->regs[i]);
	}
	__get_user(regs->hi, &context->hi);
	__get_user(regs->lo, &context->lo);

	if ((umask & 1) && context->usedfp) {
		fregs = (u64 *) &current->tss.fpu;
		for(i = 0; i < 32; i++)
			fregs[i] = (u64) context->fpregs[i];
		__get_user(current->tss.fpu.hard.control, &context->fpcsr);
	}

	/* XXX do sigstack crapola here... XXX */

	regs->orig_reg2 = -1;
	__get_user(current->blocked, &context->sigset[0]);
	current->blocked &= _BLOCKABLE;
	__get_user(res, &context->regs[2]);
	return res;

badframe:
	do_exit(SIGSEGV);
}

struct sigact_irix5 {
	int flags;
	void (*handler)(int);
	u32 sigset[4];
	int _unused0[2];
};

#ifdef DEBUG_SIG
static inline void dump_sigact_irix5(struct sigact_irix5 *p)
{
	printk("<f[%d] hndlr[%08lx] msk[%08lx]>", p->flags,
	       (unsigned long) p->handler,
	       (unsigned long) p->sigset[0]);
}
#endif

static inline void check_pending(int signum)
{
	struct sigaction *p;

	p = signum - 1 + current->sig->action;
	if (p->sa_handler == SIG_IGN) {
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

asmlinkage int irix_sigaction(int sig, struct sigact_irix5 *new,
			      struct sigact_irix5 *old, unsigned long trampoline)
{
	struct sigaction new_sa, *p;

#ifdef DEBUG_SIG
	printk(" (%d,%s,%s,%08lx) ", sig, (!new ? "0" : "NEW"),
	       (!old ? "0" : "OLD"), trampoline);
	if(new) {
		dump_sigact_irix5(new); printk(" ");
	}
#endif
	if(sig < 1 || sig > 32)
		return -EINVAL;
	p = sig - 1 + current->sig->action;

	if(new) {
		int err = verify_area(VERIFY_READ, new, sizeof(*new));
		if(err)
			return err;
		if(sig == SIGKILL || sig == SIGSTOP)
			return -EINVAL;
		new_sa.sa_flags = new->flags;
		new_sa.sa_handler = (__sighandler_t) new->handler;
		new_sa.sa_mask.__sigbits[1] = new_sa.sa_mask.__sigbits[2] =
			new_sa.sa_mask.__sigbits[3] = 0;
		new_sa.sa_mask.__sigbits[0] = new->sigset[0];

		if(new_sa.sa_handler != SIG_DFL && new_sa.sa_handler != SIG_IGN) {
			err = verify_area(VERIFY_READ, new_sa.sa_handler, 1);
			if(err)
				return err;
		}
	}
	/* Hmmm... methinks IRIX libc always passes a valid trampoline
	 * value for all invocations of sigaction.  Will have to
	 * investigate.  POSIX POSIX, die die die...
	 */
	current->tss.irix_trampoline = trampoline;
	if(old) {
		int err = verify_area(VERIFY_WRITE, old, sizeof(*old));
		if(err)
			return err;
		old->flags = p->sa_flags;
		old->handler = (void *) p->sa_handler;
		old->sigset[1] = old->sigset[2] = old->sigset[3] = 0;
		old->sigset[0] = p->sa_mask.__sigbits[0];
		old->_unused0[0] = old->_unused0[1] = 0;
	}

	if(new) {
		*p = new_sa;
		check_pending(sig);
	}

	return 0;
}

asmlinkage int irix_sigpending(unsigned long *set)
{
	int err;

	err = verify_area(VERIFY_WRITE, set, (sizeof(unsigned long) * 4));
	if(!err) {
		set[1] = set[2] = set[3] = 0;
		set[0] = (current->blocked & current->signal);
	}
	return err;
}

asmlinkage int irix_sigprocmask(int how, unsigned long *new, unsigned long *old)
{
	unsigned long bits, oldbits = current->blocked;
	int error;

	if(new) {
		error = verify_area(VERIFY_READ, new, (sizeof(unsigned long) * 4));
		if(error)
			return error;
		bits = new[0] & _BLOCKABLE;
		switch(how) {
		case 1:
			current->blocked |= bits;
			break;

		case 2:
			current->blocked &= ~bits;
			break;

		case 3:
		case 256:
			current->blocked = bits;
			break;

		default:
			return -EINVAL;
		}
	}
	if(old) {
		error = verify_area(VERIFY_WRITE, old, (sizeof(unsigned long) * 4));
		if(error)
			return error;
		old[1] = old[2] = old[3] = 0;
		old[0] = oldbits;
	}
	return 0;
}

asmlinkage int irix_sigsuspend(struct pt_regs *regs)
{
	unsigned int mask;
	unsigned long *uset;
	int base = 0;

	if(regs->regs[2] == 1000)
		base = 1;

	mask = current->blocked;
	uset = (unsigned long *) regs->regs[base + 4];
	if(verify_area(VERIFY_READ, uset, (sizeof(unsigned long) * 4)))
		return -EFAULT;
	current->blocked = uset[0] & _BLOCKABLE;
	while(1) {
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		if(do_irix_signal(mask, regs))
			return -EINTR;
	}
}

/* hate hate hate... */
struct irix5_siginfo {
	int sig, code, error;
	union {
		char unused[128 - (3 * 4)]; /* Safety net. */
		struct {
			int pid;
			union {
				int uid;
				struct {
					int utime, status, stime;
				} child;
			} procdata;
		} procinfo;

		unsigned long fault_addr;

		struct {
			int fd;
			long band;
		} fileinfo;

		unsigned long sigval;
	} stuff;
};

static inline unsigned long timespectojiffies(struct timespec *value)
{
	unsigned long sec = (unsigned) value->tv_sec;
	long nsec = value->tv_nsec;

	if (sec > (LONG_MAX / HZ))
		return LONG_MAX;
	nsec += 1000000000L / HZ - 1;
	nsec /= 1000000000L / HZ;
	return HZ * sec + nsec;
}

asmlinkage int irix_sigpoll_sys(unsigned long *set, struct irix5_siginfo *info,
				struct timespec *tp)
{
	unsigned long mask, kset, expire = 0;
	int sig, error, timeo = 0;

#ifdef DEBUG_SIG
	printk("[%s:%d] irix_sigpoll_sys(%p,%p,%p)\n",
	       current->comm, current->pid, set, info, tp);
#endif

	/* Must always specify the signal set. */
	if(!set)
		return -EINVAL;
	kset = set[0];

	error = verify_area(VERIFY_READ, set, (sizeof(unsigned long) * 4));
	if(error)
		return error;

	if(info) {
		error = verify_area(VERIFY_WRITE, info, sizeof(*info));
		if(error)
			return error;
		memset(info, 0, sizeof(*info));
	}

	if(tp) {
		error = verify_area(VERIFY_READ, tp, sizeof(*tp));
		if(error)
			return error;
		if(!tp->tv_sec && !tp->tv_nsec)
			return -EINVAL;
		expire = timespectojiffies(tp)+(tp->tv_sec||tp->tv_nsec)+jiffies;
		current->timeout = expire;
	}

	while(1) {
		current->state = TASK_INTERRUPTIBLE; schedule();
		if(current->signal & kset) break;
		if(tp && expire <= jiffies) {
			timeo = 1;
			break;
		}
		if(current->signal & ~(current->blocked)) return -EINTR;
	}

	if(timeo) return -EAGAIN;
	for(sig = 1, mask = 2; mask; mask <<= 1, sig++) {
		if(!(mask & kset)) continue;
		if(mask & current->signal) {
			/* XXX need more than this... */
			if(info) info->sig = sig;
			return 0;
		}
	}

	/* Should not get here, but do something sane if we do. */
	return -EINTR;
}

/* This is here because of irix5_siginfo definition. */
#define P_PID    0
#define P_PGID   2
#define P_ALL    7

extern int getrusage(struct task_struct *, int, struct rusage *);
extern void release(struct task_struct * p);

#define W_EXITED     1
#define W_TRAPPED    2
#define W_STOPPED    4
#define W_CONT       8
#define W_NOHANG    64

#define W_MASK      (W_EXITED | W_TRAPPED | W_STOPPED | W_CONT | W_NOHANG)

asmlinkage int irix_waitsys(int type, int pid, struct irix5_siginfo *info,
			    int options, struct rusage *ru)
{
	int flag, retval;
	struct wait_queue wait = { current, NULL };
	struct task_struct *p;

	if(!info)
		return -EINVAL;
	flag = verify_area(VERIFY_WRITE, info, sizeof(*info));
	if(flag)
		return flag;
	if(ru) {
		flag = verify_area(VERIFY_WRITE, ru, sizeof(*ru));
		if(flag)
			return flag;
	}
	if(options & ~(W_MASK))
		return -EINVAL;
	if(type != P_PID && type != P_PGID && type != P_ALL)
		return -EINVAL;
	add_wait_queue(&current->wait_chldexit, &wait);
repeat:
	flag = 0;
	for(p = current->p_cptr; p; p = p->p_osptr) {
		if((type == P_PID) && p->pid != pid)
			continue;
		if((type == P_PGID) && p->pgrp != pid)
			continue;
		if((p->exit_signal != SIGCHLD))
			continue;
		flag = 1;
		switch(p->state) {
			case TASK_STOPPED:
				if (!p->exit_code)
					continue;
				if (!(options & (W_TRAPPED|W_STOPPED)) &&
				    !(p->flags & PF_PTRACED))
					continue;
				if (ru != NULL)
					getrusage(p, RUSAGE_BOTH, ru);
				info->sig = SIGCHLD;
				info->code = 0;
				info->stuff.procinfo.pid = p->pid;
				info->stuff.procinfo.procdata.child.status =
					(p->exit_code >> 8) & 0xff;
				info->stuff.procinfo.procdata.child.utime =
					p->utime;
				info->stuff.procinfo.procdata.child.stime =
					p->stime;
				p->exit_code = 0;
				retval = 0;
				goto end_waitsys;
			case TASK_ZOMBIE:
				current->cutime += p->utime + p->cutime;
				current->cstime += p->stime + p->cstime;
				if (ru != NULL)
					getrusage(p, RUSAGE_BOTH, ru);
				info->sig = SIGCHLD;
				info->code = 1;      /* CLD_EXITED */
				info->stuff.procinfo.pid = p->pid;
				info->stuff.procinfo.procdata.child.status =
					(p->exit_code >> 8) & 0xff;
				info->stuff.procinfo.procdata.child.utime =
					p->utime;
				info->stuff.procinfo.procdata.child.stime =
					p->stime;
				retval = 0;
				if (p->p_opptr != p->p_pptr) {
					REMOVE_LINKS(p);
					p->p_pptr = p->p_opptr;
					SET_LINKS(p);
					notify_parent(p);
				} else
					release(p);
				goto end_waitsys;
			default:
				continue;
		}
	}
	if(flag) {
		retval = 0;
		if(options & W_NOHANG)
			goto end_waitsys;
		retval = -ERESTARTSYS;
		if(current->signal & ~current->blocked)
			goto end_waitsys;
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		goto repeat;
	}
	retval = -ECHILD;
end_waitsys:
	remove_wait_queue(&current->wait_chldexit, &wait);
	return retval;
}

struct irix5_context {
	u32 flags;
	u32 link;
	u32 sigmask[4];
	struct { u32 sp, size, flags; } stack;
	int regs[36];
	u32 fpregs[32];
	u32 fpcsr;
	u32 _unused0;
	u32 _unused1[47];
	u32 weird_graphics_thing;
};

asmlinkage int irix_getcontext(struct pt_regs *regs)
{
	int error, i, base = 0;
	struct irix5_context *ctx;

	if(regs->regs[2] == 1000)
		base = 1;
	ctx = (struct irix5_context *) regs->regs[base + 4];

#ifdef DEBUG_SIG
	printk("[%s:%d] irix_getcontext(%p)\n",
	       current->comm, current->pid, ctx);
#endif

	error = verify_area(VERIFY_WRITE, ctx, sizeof(*ctx));
	if(error)
		return error;
	ctx->flags = 0x0f;
	ctx->link = current->tss.irix_oldctx;
	ctx->sigmask[1] = ctx->sigmask[2] = ctx->sigmask[4] = 0;
	ctx->sigmask[0] = current->blocked;

	/* XXX Do sigstack stuff someday... */
	ctx->stack.sp = ctx->stack.size = ctx->stack.flags = 0;

	ctx->weird_graphics_thing = 0;
	ctx->regs[0] = 0;
	for(i = 1; i < 32; i++)
		ctx->regs[i] = regs->regs[i];
	ctx->regs[32] = regs->lo;
	ctx->regs[33] = regs->hi;
	ctx->regs[34] = regs->cp0_cause;
	ctx->regs[35] = regs->cp0_epc;
	if(!current->used_math) {
		ctx->flags &= ~(0x08);
	} else {
		/* XXX wheee... */
		printk("Wheee, no code for saving IRIX FPU context yet.\n");
	}
	return 0;
}

asmlinkage unsigned long irix_setcontext(struct pt_regs *regs)
{
	int error, base = 0;
	struct irix5_context *ctx;

	if(regs->regs[2] == 1000)
		base = 1;
	ctx = (struct irix5_context *) regs->regs[base + 4];

#ifdef DEBUG_SIG
	printk("[%s:%d] irix_setcontext(%p)\n",
	       current->comm, current->pid, ctx);
#endif

	error = verify_area(VERIFY_READ, ctx, sizeof(*ctx));
	if(error)
		return error;

	if(ctx->flags & 0x02) {
		/* XXX sigstack garbage, todo... */
		printk("Wheee, cannot do sigstack stuff in setcontext\n");
	}

	if(ctx->flags & 0x04) {
		int i;

		/* XXX extra control block stuff... todo... */
		for(i = 1; i < 32; i++)
			regs->regs[i] = ctx->regs[i];
		regs->lo = ctx->regs[32];
		regs->hi = ctx->regs[33];
		regs->cp0_epc = ctx->regs[35];
	}

	if(ctx->flags & 0x08) {
		/* XXX fpu context, blah... */
		printk("Wheee, cannot restore FPU context yet...\n");
	}
	current->tss.irix_oldctx = ctx->link;
	return regs->regs[2];
}

struct irix_sigstack { unsigned long sp; int status; };

asmlinkage int irix_sigstack(struct irix_sigstack *new, struct irix_sigstack *old)
{
	int error;

#ifdef DEBUG_SIG
	printk("[%s:%d] irix_sigstack(%p,%p)\n",
	       current->comm, current->pid, new, old);
#endif
	if(new) {
		error = verify_area(VERIFY_READ, new, sizeof(*new));
		if(error)
			return error;
	}

	if(old) {
		error = verify_area(VERIFY_WRITE, old, sizeof(*old));
		if(error)
			return error;
	}
	return 0;
}

struct irix_sigaltstack { unsigned long sp; int size; int status; };

asmlinkage int irix_sigaltstack(struct irix_sigaltstack *new,
				struct irix_sigaltstack *old)
{
	int error;

#ifdef DEBUG_SIG
	printk("[%s:%d] irix_sigaltstack(%p,%p)\n",
	       current->comm, current->pid, new, old);
#endif
	if(new) {
		error = verify_area(VERIFY_READ, new, sizeof(*new));
		if(error)
			return error;
	}

	if(old) {
		error = verify_area(VERIFY_WRITE, old, sizeof(*old));
		if(error)
			return error;
	}
	return 0;
}

struct irix_procset {
	int cmd, ltype, lid, rtype, rid;
};

asmlinkage int irix_sigsendset(struct irix_procset *pset, int sig)
{
	int error;

	error = verify_area(VERIFY_READ, pset, sizeof(*pset));
#ifdef DEBUG_SIG
	printk("[%s:%d] irix_sigsendset([%d,%d,%d,%d,%d],%d)\n",
	       current->comm, current->pid,
	       pset->cmd, pset->ltype, pset->lid, pset->rtype, pset->rid,
	       sig);
#endif

	return -EINVAL;
}
