/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Low level exception handling
 *
 * Copyright (C) 1998, 1999 by Ralf Baechle
 */
#ifndef _ASM_UCONTEXT_H
#define _ASM_UCONTEXT_H

typedef unsigned int greg_t;

#define NGREG 36

typedef greg_t gregset_t[NGREG];

typedef struct fpregset {
	union {
		double		fp_dregs[16];
		float		fp_fregs [32];
		unsigned int	fp_regs[32];
	} fp_r;
	unsigned int fp_csr;
	unsigned int fp_pad;
} fpregset_t;

typedef struct {
	gregset_t gregs;
	fpregset_t fpregs;
} mcontext_t;

struct ucontext {
	unsigned long	uc_flags;
	struct ucontext	*uc_link;
	stack_t		uc_stack;
	mcontext_t	uc_mcontext;
	sigset_t	uc_sigmask;	/* mask last for extensibility */
};

#endif /* _ASM_UCONTEXT_H */
