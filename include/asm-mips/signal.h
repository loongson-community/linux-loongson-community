#ifndef __ASM_MIPS_SIGNAL_H
#define __ASM_MIPS_SIGNAL_H

#if 0
/* The Linux/i386 definition */
typedef unsigned long sigset_t;		/* at least 32 bits */
#endif

#if 1
/* For now ... */
#include <asm/types.h>
typedef __u64	sigset_t;
#else
/* This is what we should really use ... */
typedef struct {
	unsigned int	sigbits[4];
} sigset_t;
#endif

#define _NSIG		65
#define NSIG		_NSIG

/*
 * For 1.3.0 Linux/MIPS changed the signal numbers to be compatible the ABI.
 */
#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGIOT		 6
#define SIGABRT		 SIGIOT
#define SIGEMT		 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGBUS		10
#define SIGSEGV		11
#define SIGSYS		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGUSR1		16
#define SIGUSR2		17
#define SIGCHLD		18
#define SIGCLD		SIGCHLD
#define SIGPWR		19
#define SIGWINCH	20
#define SIGURG		21
#define SIGIO		22
#define SIGPOLL		SIGIO
#define SIGSTOP		23
#define SIGTSTP		24
#define SIGCONT		25
#define SIGTTIN		26
#define SIGTTOU		27
#define SIGVTALRM	28
#define SIGPROF		29
#define SIGXCPU		30
#define SIGXFSZ		31

/*
 * sa_flags values: SA_STACK is not currently supported, but will allow the
 * usage of signal stacks by using the (now obsolete) sa_restorer field in
 * the sigaction structure as a stack pointer. This is now possible due to
 * the changes in signal handling. LBT 010493.
 * SA_INTERRUPT is a no-op, but left due to historical reasons. Use the
 * SA_RESTART flag to get restarting signals (which were the default long ago)
 */
#define SA_STACK	0x1
#define SA_ONSTACK	SA_STACK
#define SA_RESTART	0x4
#define SA_NOCLDSTOP	0x20000
/* Non ABI signals */
#define SA_INTERRUPT	0x01000000
#define SA_NOMASK	0x02000000
#define SA_ONESHOT	0x04000000

#define SIG_BLOCK          1	/* for blocking signals */
#define SIG_UNBLOCK        2	/* for unblocking signals */
#define SIG_SETMASK        3	/* for setting the signal mask */

/* Type of a signal handler.  */
typedef void (*__sighandler_t)(int);

#define SIG_DFL	((__sighandler_t)0)	/* default signal handling */
#define SIG_IGN	((__sighandler_t)1)	/* ignore signal */
#define SIG_ERR	((__sighandler_t)-1)	/* error return from signal */

struct sigaction {
	unsigned int	sa_flags;
	__sighandler_t	sa_handler;
	sigset_t	sa_mask;
	void		(*sa_restorer)(void);
#if __mips <= 3
	/*
	 * For 32 bit code we have to pad struct sigaction to get
	 * constant size for the ABI
	 */
	int		pad0[2];	/* reserved */
#endif
};

#ifdef __KERNEL__

/*
 * This struct isn't in the ABI, so we continue to use the old
 * pre 1.3 definition.  Needs to be changed for 64 bit kernels,
 * but it's 4am ...
 */
struct sigcontext_struct {
	unsigned long	       sc_at, sc_v0, sc_v1, sc_a0, sc_a1, sc_a2, sc_a3;
	unsigned long	sc_t0, sc_t1, sc_t2, sc_t3, sc_t4, sc_t5, sc_t6, sc_t7;
	unsigned long	sc_s0, sc_s1, sc_s2, sc_s3, sc_s4, sc_s5, sc_s6, sc_s7;
	unsigned long	sc_t8, sc_t9,               sc_gp, sc_sp, sc_fp, sc_ra;

	unsigned long	sc_epc;
	unsigned long	sc_cause;

	unsigned long	oldmask;
};

#endif

#endif /* __ASM_MIPS_SIGNAL_H */
