/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 95, 96, 97, 98, 99, 2000 by Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 */
#ifndef _ASM_PTRACE_H
#define _ASM_PTRACE_H

/* 0 - 31 are integer registers, 32 - 63 are fp registers.  */
#define FPR_BASE	32
#define PC		64
#define CAUSE		65
#define BADVADDR	66
#define MMHI		67
#define MMLO		68
#define FPC_CSR		69
#define FPC_EIR		70

#ifndef __ASSEMBLY__

/*
 * This struct defines the way the registers are stored on the stack during a
 * system call/exception. As usual the registers k0/k1 aren't being saved.
 */
struct pt_regs {
	/* Saved main processor registers. */
	unsigned long regs[32];

	/* Other saved registers. */
	unsigned long lo;
	unsigned long hi;

	/*
	 * saved cp0 registers
	 */
	unsigned long cp0_epc;
	unsigned long cp0_badvaddr;
	unsigned long cp0_status;
	unsigned long cp0_cause;
};

#define __str2(x) #x
#define __str(x) __str2(x)

/* Used in declaration of save_static functions.  */
#define static_unused static __attribute__((unused))

#define save_static_function(symbol)                                    \
__asm__ (                                                               \
	".text\n\t"							\
        ".globl\t" #symbol "\n\t"                                       \
        ".align\t2\n\t"                                                 \
        ".type\t" #symbol ", @function\n\t"                             \
        ".ent\t" #symbol ", 0\n"                                        \
        #symbol":\n\t"                                                  \
        ".frame\t$29, 0, $31\n\t"                                       \
        ".end\t" #symbol "\n\t"                                         \
        ".size\t" #symbol",. - " #symbol)

#define save_static(frame)                               \
	__asm__ __volatile__(                            \
		"sd\t$16,"__str(PT_R16)"(%0)\n\t"        \
		"sd\t$17,"__str(PT_R17)"(%0)\n\t"        \
		"sd\t$18,"__str(PT_R18)"(%0)\n\t"        \
		"sd\t$19,"__str(PT_R19)"(%0)\n\t"        \
		"sd\t$20,"__str(PT_R20)"(%0)\n\t"        \
		"sd\t$21,"__str(PT_R21)"(%0)\n\t"        \
		"sd\t$22,"__str(PT_R22)"(%0)\n\t"        \
		"sd\t$23,"__str(PT_R23)"(%0)\n\t"        \
		"sd\t$30,"__str(PT_R30)"(%0)\n\t"        \
		: /* No outputs */                       \
		: "r" (frame))

#define nabi_no_regargs							\
	unsigned long __dummy0,						\
	unsigned long __dummy1,						\
	unsigned long __dummy2,						\
	unsigned long __dummy3,						\
	unsigned long __dummy4,						\
	unsigned long __dummy5,						\
	unsigned long __dummy6,						\
	unsigned long __dummy7,

#endif /* !__ASSEMBLY__ */

/* Arbitrarily choose the same ptrace numbers as used by the Sparc code. */
/* #define PTRACE_GETREGS		12 */
/* #define PTRACE_SETREGS		13 */
/* #define PTRACE_GETFPREGS		14 */
/* #define PTRACE_SETFPREGS		15 */
/* #define PTRACE_GETFPXREGS		18 */
/* #define PTRACE_SETFPXREGS		19 */

#define PTRACE_OLDSETOPTIONS	21

#define PTRACE_GET_THREAD_AREA	25
#define PTRACE_SET_THREAD_AREA	26

#ifdef __ASSEMBLY__
#include <asm/offset.h>
#endif /* !__ASSEMBLY__ */

#ifdef __KERNEL__

#ifndef __ASSEMBLY__
#define instruction_pointer(regs) ((regs)->cp0_epc)

extern void show_regs(struct pt_regs *);
#endif /* !__ASSEMBLY__ */

#endif

#endif /* _ASM_PTRACE_H */
