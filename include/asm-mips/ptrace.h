/*
 * linux/include/asm-mips/ptrace.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995, 1996 by Ralf Baechle
 *
 * Machine dependent structs and defines to help the user use
 * the ptrace system call.
 */
#ifndef __ASM_MIPS_PTRACE_H
#define __ASM_MIPS_PTRACE_H

#include <asm/sgidefs.h>
#include <asm/register.h>

/*
 * General purpose registers.
 */
#define GPR_BASE        0
#define NGP_REGS        32

/*
 * Floating point registers.
 */
#define FPR_BASE        (GPR_BASE + NGP_REGS)
#define NFP_REGS        32

/*
 * Signal handlers
 */
#define SIG_BASE        (FPR_BASE + NFP_REGS)
#define NSIG_HNDLRS     32

/*
 * Special registers.
 */
#define SPEC_BASE       (SIG_BASE + NSIG_HNDLRS)
#define NSPEC_REGS      7
#define PC              SPEC_BASE
#define CAUSE           (PC       + 1)
#define BADVADDR        (CAUSE    + 1)
#define MMHI            (BADVADDR + 1)
#define MMLO            (MMHI     + 1)
#define FPC_CSR         (MMLO     + 1)
#define FPC_EIR         (FPC_CSR  + 1)
#define NPTRC_REGS      (FPC_EIR  + 1)

/*
 * This struct defines the way the registers are stored on the stack during a
 * system call/exception. As usual the registers zero/k0/k1 aren't being saved.
 */
struct pt_regs {
	/*
	 * Pad bytes for argument save space on the stack
	 * 20/40 Bytes for 32/64 bit code
	 */
	__register_t pad0[6];	/* extra word for alignment */

	/*
	 * Saved main processor registers
	 */
	__register_t	regs[32];

	/*
	 * Other saved registers
	 */
	__register_t	lo;
	__register_t	hi;
	__register_t	orig_reg2;
	__register_t	orig_reg7;

	/*
	 * saved cp0 registers
	 */
	__register_t	cp0_epc;
	__register_t	cp0_badvaddr;
	unsigned int	cp0_status;
	unsigned int	cp0_cause;
};

#ifdef __KERNEL__

/*
 * Does the process account for user or for system time?
 */
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
#define user_mode(regs) (!((regs)->cp0_status & 0x8))
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS5)
#define user_mode(regs) ((regs)->cp0_status & 0x10)
#endif

#define instruction_pointer(regs) ((regs)->cp0_epc)
extern void show_regs(struct pt_regs *);
#endif

#endif /* __ASM_MIPS_PTRACE_H */
