/*
 *  include/asm-mips/stackframe.h
 *
 *  Copyright (C) 1994  Waldorf GMBH
 *  written by Ralf Baechle
 */

#ifndef _ASM_MIPS_STACKFRAME_H_
#define _ASM_MIPS_STACKFRAME_H_

/*
 * Stack layout for all exceptions:
 *
 * ptrace needs to have all regs on the stack.
 * if the order here is changed, it needs to be 
 * updated in asm/mips/fork.c:copy_process, asm/mips/signal.c:do_signal,
 * asm-mips/ptrace.c, include/asm-mips/ptrace.h
 * and asm-mips/ptrace
 */

/*
 * Offsets into the Interrupt stackframe.
 */
#define FR_REG1		0
#define FR_REG2		4
#define FR_REG3		8
#define FR_REG4		12
#define FR_REG5		16
#define FR_REG6		20
#define FR_REG7		24
#define FR_REG8		28
#define FR_REG9		32
#define FR_REG10	36
#define FR_REG11	40
#define FR_REG12	44
#define FR_REG13	48
#define FR_REG14	52
#define FR_REG15	56
#define FR_REG16	60
#define FR_REG17	64
#define FR_REG18	68
#define FR_REG19	72
#define FR_REG20	76
#define FR_REG21	80
#define FR_REG22	84
#define FR_REG23	88
#define FR_REG24	92
#define FR_REG25	96

/*
 * $26 (k0) and $27 (k1) not saved
 */
#define FR_REG28	100
#define FR_REG29	104
#define FR_REG30	108
#define FR_REG31	112

/*
 * Saved special registers
 */
#define FR_LO		((FR_REG31) + 4)
#define FR_HI		((FR_LO) + 4)

/*
 * Saved cp0 registers follow
 */
#define FR_STATUS	((FR_HI) + 4)
#define FR_EPC		((FR_STATUS) + 4)
#define FR_ERROREPC	((FR_EPC) + 4)

/*
 * Some goodies...
 */
#define FR_INTERRUPT	((FR_ERROREPC) + 4)
#define FR_ORIG_REG2	((FR_INTERRUPT) + 4)

/*
 * Size of stack frame
 */
#define FR_SIZE		((FR_ORIG_REG2) + 4)

#define SAVE_ALL \
		mfc0	k0,CP0_STATUS; \
		andi	k0,k0,0x18;		/* extract KSU bits */ \
		beq	zero,k0,1f; \
		move	k1,sp; \
		/* \
		 * Called from user mode, new stack \
		 */ \
		lw	k1,_kernelsp; \
1:		move	k0,sp; \
		subu	sp,k1,FR_SIZE; \
		sw	k0,FR_REG29(sp); \
		sw	$2,FR_REG2(sp); \
		sw	$2,FR_ORIG_REG2(sp); \
		mfc0	v0,CP0_STATUS; \
		sw	v0,FR_STATUS(sp); \
		mfc0	v0,CP0_EPC; \
		sw	v0,FR_EPC(sp); \
		mfc0	v0,CP0_ERROREPC; \
		sw	v0,FR_ERROREPC(sp); \
		mfhi	v0; \
		sw	v0,FR_HI(sp); \
		mflo	v0; \
		sw	v0,FR_LO(sp); \
		sw	$1,FR_REG1(sp); \
		sw	$3,FR_REG3(sp); \
		sw	$4,FR_REG4(sp); \
		sw	$5,FR_REG5(sp); \
		sw	$6,FR_REG6(sp); \
		sw	$7,FR_REG7(sp); \
		sw	$8,FR_REG8(sp); \
		sw	$9,FR_REG9(sp); \
		sw	$10,FR_REG10(sp); \
		sw	$11,FR_REG11(sp); \
		sw	$12,FR_REG12(sp); \
		sw	$13,FR_REG13(sp); \
		sw	$14,FR_REG14(sp); \
		sw	$15,FR_REG15(sp); \
		sw	$16,FR_REG16(sp); \
		sw	$17,FR_REG17(sp); \
		sw	$18,FR_REG18(sp); \
		sw	$19,FR_REG19(sp); \
		sw	$20,FR_REG20(sp); \
		sw	$21,FR_REG21(sp); \
		sw	$22,FR_REG22(sp); \
		sw	$23,FR_REG23(sp); \
		sw	$24,FR_REG24(sp); \
		sw	$25,FR_REG25(sp); \
		sw	$28,FR_REG28(sp); \
		sw	$30,FR_REG30(sp); \
		sw	$31,FR_REG31(sp)

#define RESTORE_ALL \
		lw	v0,FR_ERROREPC(sp); \
		lw	v1,FR_EPC(sp); \
		mtc0	v0,CP0_ERROREPC; \
		lw	v0,FR_HI(sp); \
		mtc0	v1,CP0_EPC; \
		lw	v1,FR_LO(sp); \
		mthi	v0; \
		lw	v0,FR_STATUS(sp); \
		mtlo	v1; \
		mtc0	v0,CP0_STATUS; \
		lw	$31,FR_REG31(sp); \
		lw	$30,FR_REG30(sp); \
		lw	$28,FR_REG28(sp); \
		lw	$25,FR_REG25(sp); \
		lw	$24,FR_REG24(sp); \
		lw	$23,FR_REG23(sp); \
		lw	$22,FR_REG22(sp); \
		lw	$21,FR_REG21(sp); \
		lw	$20,FR_REG20(sp); \
		lw	$19,FR_REG19(sp); \
		lw	$18,FR_REG18(sp); \
		lw	$17,FR_REG17(sp); \
		lw	$16,FR_REG16(sp); \
		lw	$15,FR_REG15(sp); \
		lw	$14,FR_REG14(sp); \
		lw	$13,FR_REG13(sp); \
		lw	$12,FR_REG12(sp); \
		lw	$11,FR_REG11(sp); \
		lw	$10,FR_REG10(sp); \
		lw	$9,FR_REG9(sp); \
		lw	$8,FR_REG8(sp); \
		lw	$7,FR_REG7(sp); \
		lw	$6,FR_REG6(sp); \
		lw	$5,FR_REG5(sp); \
		lw	$4,FR_REG4(sp); \
		lw	$3,FR_REG3(sp); \
		lw	$2,FR_REG2(sp); \
		lw	$1,FR_REG1(sp); \
		lw	sp,FR_REG29(sp); /* Deallocate stack */ \
		eret

#endif /* _ASM_MIPS_STACKFRAME_H_ */
