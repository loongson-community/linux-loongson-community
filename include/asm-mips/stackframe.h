/*
 * Makefile for MIPS Linux main source directory
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995, 1996 by Ralf Baechle and Paul M. Antoine.
 * Additional R3000 support by Didier Frick <dfrick@dial.eunet.ch>
 * for ACN S.A, Copyright (C) 1996 by ACN S.A
 */
#ifndef __ASM_MIPS_STACKFRAME_H
#define __ASM_MIPS_STACKFRAME_H

#include <asm/sgidefs.h>
#include <asm/asm.h>

/*
 * Stack layout for all exceptions:
 *
 * ptrace needs to have all regs on the stack. If the order here is changed,
 * it needs to be updated in include/asm-mips/ptrace.h
 *
 * The first PTRSIZE*6 bytes are argument save space for C subroutines.
 * zero doesn't get saved; it's just a placeholder.
 */
#define FR_REG0		(SZREG*6)
#define FR_REG1		((FR_REG0) + SZREG)
#define FR_REG2		((FR_REG1) + SZREG)
#define FR_REG3		((FR_REG2) + SZREG)
#define FR_REG4		((FR_REG3) + SZREG)
#define FR_REG5		((FR_REG4) + SZREG)
#define FR_REG6		((FR_REG5) + SZREG)
#define FR_REG7		((FR_REG6) + SZREG)
#define FR_REG8		((FR_REG7) + SZREG)
#define FR_REG9		((FR_REG8) + SZREG)
#define FR_REG10	((FR_REG9) + SZREG)
#define FR_REG11	((FR_REG10) + SZREG)
#define FR_REG12	((FR_REG11) + SZREG)
#define FR_REG13	((FR_REG12) + SZREG)
#define FR_REG14	((FR_REG13) + SZREG)
#define FR_REG15	((FR_REG14) + SZREG)
#define FR_REG16	((FR_REG15) + SZREG)
#define FR_REG17	((FR_REG16) + SZREG)
#define FR_REG18	((FR_REG17) + SZREG)
#define FR_REG19	((FR_REG18) + SZREG)
#define FR_REG20	((FR_REG19) + SZREG)
#define FR_REG21	((FR_REG20) + SZREG)
#define FR_REG22	((FR_REG21) + SZREG)
#define FR_REG23	((FR_REG22) + SZREG)
#define FR_REG24	((FR_REG23) + SZREG)
#define FR_REG25	((FR_REG24) + SZREG)

/*
 * $26 (k0) and $27 (k1) not saved - just placeholders
 */
#define FR_REG26	((FR_REG25) + SZREG)
#define FR_REG27	((FR_REG26) + SZREG)

#define FR_REG28	((FR_REG27) + SZREG)
#define FR_REG29	((FR_REG28) + SZREG)
#define FR_REG30	((FR_REG29) + SZREG)
#define FR_REG31	((FR_REG30) + SZREG)

/*
 * Saved special registers
 */
#define FR_LO		((FR_REG31) + SZREG)
#define FR_HI		((FR_LO) + SZREG)
#define FR_ORIG_REG2	((FR_HI) + SZREG)
#define FR_ORIG_REG7	((FR_ORIG_REG2) + SZREG)

/*
 * Saved cp0 registers follow
 */
#define FR_EPC		((FR_ORIG_REG7) + SZREG)
#define FR_BADVADDR	((FR_EPC) + SZREG)
#define FR_STATUS	((FR_BADVADDR) + SZREG)
#define FR_CAUSE	((FR_STATUS) + 4)

/*
 * Size of stack frame, word/double word alignment
 */
#define FR_SIZE		(((FR_CAUSE + 4) + ALSZ) & ALMASK)

/*
 * Load the global pointer.  Only for ELF executables global pointer
 * optimization is possible, so we only load the global pointer for
 * ELF kernels.
 */
#if 0
#define LOAD_GP	la	gp,_gp
#else
#define LOAD_GP
#endif

#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)

#define SAVE_ALL                                        \
		mfc0	k0,CP0_STATUS;                  \
                li      k1,ST0_CU0;                     \
		and	k0,k1;                          \
		bnez	k0,8f;                          \
		move	k1,sp;                          \
                nop;                                    \
		/*                                      \
		 * Called from user mode, new stack     \
		 */                                     \
		lw	k1,kernelsp;                    \
8:		move	k0,sp;                          \
		subu	sp,k1,FR_SIZE;                  \
		sw	k0,FR_REG29(sp);                \
		sw	$3,FR_REG3(sp);                 \
		mfc0	$3,CP0_STATUS;                  \
		nop;					\
		sw	$3,FR_STATUS(sp);               \
		li	k1,ST0_CU0;                     \
		or	k1,$3;                          \
		mtc0	k1,CP0_STATUS;                  \
		nop;					\
		mfc0	$3,CP0_ENTRYHI;                 \
		nop;					\
		sw	$3,FR_ENTRYHI(sp);              \
		mfc0	$3,CP0_CAUSE;                   \
		nop;					\
		sw	$3,FR_CAUSE(sp);                \
		mfc0	$3,CP0_EPC;                     \
		nop;					\
		sw	$3,FR_EPC(sp);                  \
		mfc0	$3,CP0_BADVADDR;                \
		nop;					\
		sw	$3,FR_BADVADDR(sp);             \
		mfhi	$3;                             \
		sw	$3,FR_HI(sp);                   \
		mflo	$3;                             \
		sw	$3,FR_LO(sp);                   \
		sw	$1,FR_REG1(sp);                 \
		sw	$2,FR_REG2(sp);                 \
		sw	$4,FR_REG4(sp);                 \
		sw	$5,FR_REG5(sp);                 \
		sw	$6,FR_REG6(sp);                 \
		sw	$7,FR_REG7(sp);                 \
		sw	$8,FR_REG8(sp);                 \
		sw	$9,FR_REG9(sp);                 \
		sw	$10,FR_REG10(sp);               \
		sw	$11,FR_REG11(sp);               \
		sw	$12,FR_REG12(sp);               \
		sw	$13,FR_REG13(sp);               \
		sw	$14,FR_REG14(sp);               \
		sw	$15,FR_REG15(sp);               \
		sw	$16,FR_REG16(sp);               \
		sw	$17,FR_REG17(sp);               \
		sw	$18,FR_REG18(sp);               \
		sw	$19,FR_REG19(sp);               \
		sw	$20,FR_REG20(sp);               \
		sw	$21,FR_REG21(sp);               \
		sw	$22,FR_REG22(sp);               \
		sw	$23,FR_REG23(sp);               \
		sw	$24,FR_REG24(sp);               \
		sw	$25,FR_REG25(sp);               \
		sw	$28,FR_REG28(sp);               \
		sw	$30,FR_REG30(sp);               \
		sw	$31,FR_REG31(sp);               \
		LOAD_GP

/*
 * Note that we restore the IE flags from stack. This means
 * that a modified IE mask will be nullified.
 */
/*
 * FIXME: Don't need to clear these bits on R[236]000's??
 *
		mfc0	t0,CP0_STATUS;                  \
		ori	t0,0x1f;                        \
		xori	t0,0x1f;                        \
		mtc0	t0,CP0_STATUS;                  \
 */
#define RESTORE_ALL                                     \
		lw	v1,FR_LO(sp);                   \
		lw	v0,FR_HI(sp);                   \
		mtlo	v1;                             \
		lw	v1,FR_EPC(sp);                  \
		mthi	v0;                             \
		mtc0	v1,CP0_EPC;                     \
		lw	$31,FR_REG31(sp);               \
		lw	$30,FR_REG30(sp);               \
		lw	$28,FR_REG28(sp);               \
		lw	$25,FR_REG25(sp);               \
		lw	$24,FR_REG24(sp);               \
		lw	$23,FR_REG23(sp);               \
		lw	$22,FR_REG22(sp);               \
		lw	$21,FR_REG21(sp);               \
		lw	$20,FR_REG20(sp);               \
		lw	$19,FR_REG19(sp);               \
		lw	$18,FR_REG18(sp);               \
		lw	$17,FR_REG17(sp);               \
		lw	$16,FR_REG16(sp);               \
		lw	$15,FR_REG15(sp);               \
		lw	$14,FR_REG14(sp);               \
		lw	$13,FR_REG13(sp);               \
		lw	$12,FR_REG12(sp);               \
		lw	$11,FR_REG11(sp);               \
		lw	$10,FR_REG10(sp);               \
		lw	$9,FR_REG9(sp);                 \
		lw	$8,FR_REG8(sp);                 \
		lw	$7,FR_REG7(sp);                 \
		lw	$6,FR_REG6(sp);                 \
		lw	$5,FR_REG5(sp);                 \
		lw	$4,FR_REG4(sp);                 \
		lw	$3,FR_REG3(sp);                 \
		lw	$2,FR_REG2(sp);                 \
		lw	$1,FR_REG1(sp);                 \
		lw	k1,FR_STATUS(sp);               \
		lw	sp,FR_REG29(sp);                \
                ori     k1,3;                           \
                xori    k1,3;                           \
                mtc0    k1,CP0_STATUS;                  \
                nop

/*
 * We disable interrupts when restoring the status register because:
 * 1) the ret_from_syscall routine uses k0/k1 to preserve values around
 *    the RESTORE_ALL
 * 2) the rfe instruction will restore the IE and KU flags to their
 *    previous value.
 */

#define CLI                                             \
		mfc0	t1,$12;				\
		li	t0,ST0_CU0|1;			\
		or	t1,t1,t0;			\
		xori	t1,1;				\
		mtc0	t1,$12;				\
		nop;					\
		nop

#define STI						\
		mfc0	t1,$12;				\
		li	t0,ST0_CU0|1;			\
		or	t1,t1,t0;			\
		mtc0	t1,$12;				\
		nop;					\
		nop

#endif /* (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2) */

#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS5)

#define SAVE_ALL                                        \
		mfc0	k0,CP0_STATUS;                  \
		sll	k0,3;     /* extract cu0 bit */ \
		bltz	k0,8f;                          \
		move	k1,sp;                          \
		/*                                      \
		 * Called from user mode, new stack     \
		 */                                     \
		lw	k1,kernelsp;                    \
8:		move	k0,sp;                          \
		subu	sp,k1,FR_SIZE;                  \
		sd	k0,FR_REG29(sp);                \
		sd	$3,FR_REG3(sp);                 \
		mfc0	$3,CP0_STATUS;                  \
		sw	$3,FR_STATUS(sp);               \
		mfc0	$3,CP0_CAUSE;                   \
		sw	$3,FR_CAUSE(sp);                \
		dmfc0	$3,CP0_EPC;                     \
		sd	$3,FR_EPC(sp);                  \
		mfhi	$3;                             \
		sd	$3,FR_HI(sp);                   \
		mflo	$3;                             \
		sd	$3,FR_LO(sp);                   \
		sd	$1,FR_REG1(sp);                 \
		sd	$2,FR_REG2(sp);                 \
		sd	$4,FR_REG4(sp);                 \
		sd	$5,FR_REG5(sp);                 \
		sd	$6,FR_REG6(sp);                 \
		sd	$7,FR_REG7(sp);                 \
		sd	$8,FR_REG8(sp);                 \
		sd	$9,FR_REG9(sp);                 \
		sd	$10,FR_REG10(sp);               \
		sd	$11,FR_REG11(sp);               \
		sd	$12,FR_REG12(sp);               \
		sd	$13,FR_REG13(sp);               \
		sd	$14,FR_REG14(sp);               \
		sd	$15,FR_REG15(sp);               \
		sd	$16,FR_REG16(sp);               \
		sd	$17,FR_REG17(sp);               \
		sd	$18,FR_REG18(sp);               \
		sd	$19,FR_REG19(sp);               \
		sd	$20,FR_REG20(sp);               \
		sd	$21,FR_REG21(sp);               \
		sd	$22,FR_REG22(sp);               \
		sd	$23,FR_REG23(sp);               \
		sd	$24,FR_REG24(sp);               \
		sd	$25,FR_REG25(sp);               \
		sd	$28,FR_REG28(sp);               \
		sd	$30,FR_REG30(sp);               \
		sd	$31,FR_REG31(sp);               \
		LOAD_GP

/*
 * Note that we restore the IE flags from stack. This means
 * that a modified IE mask will be nullified.
 */
#define RESTORE_ALL                                     \
		mfc0	t0,CP0_STATUS;                  \
		ori	t0,0x1f;                        \
		xori	t0,0x1f;                        \
		mtc0	t0,CP0_STATUS;                  \
		\
		lw	v0,FR_STATUS(sp);               \
		ld	v1,FR_LO(sp);                   \
		mtc0	v0,CP0_STATUS;                  \
		mtlo	v1;                             \
		ld	v0,FR_HI(sp);                   \
		ld	v1,FR_EPC(sp);                  \
		mthi	v0;                             \
		dmtc0	v1,CP0_EPC;                     \
		ld	$31,FR_REG31(sp);               \
		ld	$30,FR_REG30(sp);               \
		ld	$28,FR_REG28(sp);               \
		ld	$25,FR_REG25(sp);               \
		ld	$24,FR_REG24(sp);               \
		ld	$23,FR_REG23(sp);               \
		ld	$22,FR_REG22(sp);               \
		ld	$21,FR_REG21(sp);               \
		ld	$20,FR_REG20(sp);               \
		ld	$19,FR_REG19(sp);               \
		ld	$18,FR_REG18(sp);               \
		ld	$17,FR_REG17(sp);               \
		ld	$16,FR_REG16(sp);               \
		ld	$15,FR_REG15(sp);               \
		ld	$14,FR_REG14(sp);               \
		ld	$13,FR_REG13(sp);               \
		ld	$12,FR_REG12(sp);               \
		ld	$11,FR_REG11(sp);               \
		ld	$10,FR_REG10(sp);               \
		ld	$9,FR_REG9(sp);                 \
		ld	$8,FR_REG8(sp);                 \
		ld	$7,FR_REG7(sp);                 \
		ld	$6,FR_REG6(sp);                 \
		ld	$5,FR_REG5(sp);                 \
		ld	$4,FR_REG4(sp);                 \
		ld	$3,FR_REG3(sp);                 \
		ld	$2,FR_REG2(sp);                 \
		ld	$1,FR_REG1(sp);                 \
		ld	sp,FR_REG29(sp) /* Deallocate stack */ \

/*
 * Move to kernel mode and disable interrupts.
 * Set cp0 enable bit as sign that we're running on the kernel stack
 */
#define CLI                                             \
		mfc0	t0,CP0_STATUS;                  \
		li	t1,ST0_CU0|0x1f;                \
		or	t0,t1;                          \
		xori	t0,0x1f;                        \
		mtc0	t0,CP0_STATUS

/*
 * Move to kernel mode and enable interrupts.
 * Set cp0 enable bit as sign that we're running on the kernel stack
 *
 * Note that the mtc0 will be effective on R4000 pipeline stage 7. This
 * means that another three instructions will be executed with interrupts
 * disabled.  Arch/mips/mips3/r4xx0.S makes use of this fact.
 */
#define STI                                             \
		mfc0	t0,CP0_STATUS;                  \
		li	t1,ST0_CU0|0x1f;                \
		or	t0,t1;                          \
		xori	t0,0x1e;                        \
		mtc0	t0,CP0_STATUS

#endif /* _MIPS_ISA >= MIPS3 */

#endif /* __ASM_MIPS_STACKFRAME_H */
