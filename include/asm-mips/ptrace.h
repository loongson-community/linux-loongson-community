/*
 * linux/include/asm-mips/ptrace.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 by Waldorf GMBH
 * written by Ralf Baechle
 *
 * Machine dependent structs and defines to help the user use
 * the ptrace system call.
 */
#ifndef __ASM_MIPS_PTRACE_H
#define __ASM_MIPS_PTRACE_H

/*
 * This defines/structures correspond to the register layout on stack -
 * if the order here is changed, it needs to be updated in
 * include/asm-mips/stackframe.h
 */
#define IN_REG1       5
#define IN_REG2       6
#define IN_REG3       7
#define IN_REG4       8
#define IN_REG5       9
#define IN_REG6       10
#define IN_REG7       11
#define IN_REG8       12
#define IN_REG9       13
#define IN_REG10      14
#define IN_REG11      15
#define IN_REG12      16
#define IN_REG13      17
#define IN_REG14      18
#define IN_REG15      19
#define IN_REG16      20
#define IN_REG17      21
#define IN_REG18      22
#define IN_REG19      23
#define IN_REG20      24
#define IN_REG21      25
#define IN_REG22      26
#define IN_REG23      27
#define IN_REG24      28
#define IN_REG25      29
/*
 * k0/k1 unsaved
 */
#define IN_REG28      30
#define IN_REG29      31
#define IN_REG30      32
#define IN_REG31      33

/*
 * Saved special registers
 */
#define IN_LO         34
#define IN_HI         35

/*
 * saved cp0 registers
 */
#define IN_CP0_STATUS 36
#define IN_CP0_EPC    37
#define IN_CP0_CAUSE  38

/*
 * Some goodies
 */
#define IN_INTERRUPT  39
#define IN_ORIG_REG2  40

/*
 * This struct defines the way the registers are stored on the stack during a
 * system call/exception. As usual the registers k0/k1 aren't being saved.
 */
struct pt_regs {
	/*
	 * Pad bytes for argument save space on the stack
	 * 20/40 Bytes for 32/64 bit code
	 */
	unsigned long pad0[5];

	/*
	 * saved main processor registers
	 */
	long	        reg1,  reg2,  reg3,  reg4,  reg5,  reg6,  reg7;
	long	 reg8,  reg9, reg10, reg11, reg12, reg13, reg14, reg15;
	long	reg16, reg17, reg18, reg19, reg20, reg21, reg22, reg23;
	long	reg24, reg25,               reg28, reg29, reg30, reg31;

	/*
	 * Saved special registers
	 */
	long	lo;
	long	hi;

	/*
	 * saved cp0 registers
	 */
	unsigned long cp0_status;
	unsigned long cp0_epc;
	unsigned long cp0_cause;

	/*
	 * Some goodies...
	 */
	unsigned long interrupt;
	long orig_reg2;
	long pad1;
};

#ifdef __KERNEL__

/*
 * Does the process account for user or for system time?
 */
#if defined (__R4000__)

#define user_mode(regs) ((regs)->cp0_status & 0x10)

#else /* !defined (__R4000__) */

#define user_mode(regs) (!((regs)->cp0_status & 0x8))

#endif /* !defined (__R4000__) */

extern void show_regs(struct pt_regs *);
#endif

#endif /* __ASM_MIPS_PTRACE_H */
