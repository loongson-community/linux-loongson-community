/*
 * Save/restore floating point context for signal handlers.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by Ralf Baechle
 */
#include <asm/asm.h>
#include <asm/fpregdef.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/sigcontext.h>

#define SDC1(r,m)                                  \
7:		sdc1	r,m;                       \
		.section	__ex_table,"a";    \
		PTR	7b,bad_stack;              \
		.text

#define SW(r,m)                                    \
7:		sw	r,m;                       \
		.section	__ex_table,"a";    \
		PTR	7b,bad_stack;              \
		.text

#define LDC1(r,m)                                  \
7:		ldc1	r,m;                       \
		.section	__ex_table,"a";    \
		PTR	7b,bad_stack;              \
		.text

#define LW(r,m)                                    \
7:		lw	r,m;                       \
		.section	__ex_table,"a";    \
		PTR	7b,bad_stack;              \
		.text

		.set	noreorder
/*
 * Save floating point context
 */
LEAF(mips3_save_fp_context)
		mfc0	t1,CP0_STATUS
		sll	t2,t1,2
		bgez	t2,2f
		sll	t2,t1,5				# delay slot
		bgez	t2,1f
		cfc1	t1,fcr31			# delay slot
		/*
		 * Store the 16 odd double precision registers
		 */
		SDC1	($f1,(SC_FPREGS+8)(a0))
		SDC1	($f3,(SC_FPREGS+24)(a0))
		SDC1	($f5,(SC_FPREGS+40)(a0))
		SDC1	($f7,(SC_FPREGS+56)(a0))
		SDC1	($f9,(SC_FPREGS+72)(a0))
		SDC1	($f11,(SC_FPREGS+88)(a0))
		SDC1	($f13,(SC_FPREGS+104)(a0))
		SDC1	($f15,(SC_FPREGS+120)(a0))
		SDC1	($f17,(SC_FPREGS+136)(a0))
		SDC1	($f19,(SC_FPREGS+152)(a0))
		SDC1	($f21,(SC_FPREGS+168)(a0))
		SDC1	($f23,(SC_FPREGS+184)(a0))
		SDC1	($f25,(SC_FPREGS+200)(a0))
		SDC1	($f27,(SC_FPREGS+216)(a0))
		SDC1	($f29,(SC_FPREGS+232)(a0))
		SDC1	($f31,(SC_FPREGS+248)(a0))

		/*
		 * Store the 16 even double precision registers
		 */
1:		SDC1	($f0,(SC_FPREGS+0)(a0))
		SDC1	($f2,(SC_FPREGS+16)(a0))
		SDC1	($f4,(SC_FPREGS+32)(a0))
		SDC1	($f6,(SC_FPREGS+48)(a0))
		SDC1	($f8,(SC_FPREGS+64)(a0))
		SDC1	($f10,(SC_FPREGS+80)(a0))
		SDC1	($f12,(SC_FPREGS+96)(a0))
		SDC1	($f14,(SC_FPREGS+112)(a0))
		SDC1	($f16,(SC_FPREGS+128)(a0))
		SDC1	($f18,(SC_FPREGS+144)(a0))
		SDC1	($f20,(SC_FPREGS+160)(a0))
		SDC1	($f22,(SC_FPREGS+176)(a0))
		SDC1	($f24,(SC_FPREGS+192)(a0))
		SDC1	($f26,(SC_FPREGS+208)(a0))
		SDC1	($f28,(SC_FPREGS+224)(a0))
		SDC1	($f30,(SC_FPREGS+240)(a0))
		SW	t1,SC_FPC_CSR(a0)
		cfc1	t0,$0				# implementation/version
		jr	ra
		.set	nomacro
		SW	t1,SC_FPC_EIR(a0)		# delay slot
		.set	macro

2:		jr	ra
		.set	nomacro
		nop					# delay slot
		.set	macro
		END(mips3_save_fp_context)

/*
 * Restore fpu state:
 *  - fp gp registers
 *  - cp1 status/control register
 *
 * We base the decission which registers to restore from the signal stack
 * frame on the current content of c0_status, not on the content of the
 * stack frame which might have been changed by the user.
 */
LEAF(mips3_restore_fp_context)
		mfc0	t1,CP0_STATUS
		sll	t0,t1,2
		bgez	t0,2f
		sll	t0,t1,5				# delay slot
		bgez	t0,1f
		LW	t0,SC_FPC_CSR(a0)		# delay slot
		/*
		 * Restore the 16 odd double precision registers only
		 * when enabled in the cp0 status register.
		 */
		LDC1	($f1,(SC_FPREGS+8)(a0))
		LDC1	($f3,(SC_FPREGS+24)(a0))
		LDC1	($f5,(SC_FPREGS+40)(a0))
		LDC1	($f7,(SC_FPREGS+56)(a0))
		LDC1	($f9,(SC_FPREGS+72)(a0))
		LDC1	($f11,(SC_FPREGS+88)(a0))
		LDC1	($f13,(SC_FPREGS+104)(a0))
		LDC1	($f15,(SC_FPREGS+120)(a0))
		LDC1	($f17,(SC_FPREGS+136)(a0))
		LDC1	($f19,(SC_FPREGS+152)(a0))
		LDC1	($f21,(SC_FPREGS+168)(a0))
		LDC1	($f23,(SC_FPREGS+184)(a0))
		LDC1	($f25,(SC_FPREGS+200)(a0))
		LDC1	($f27,(SC_FPREGS+216)(a0))
		LDC1	($f29,(SC_FPREGS+232)(a0))
		LDC1	($f31,(SC_FPREGS+248)(a0))

		/*
		 * Restore the 16 even double precision registers
		 * when cp1 was enabled in the cp0 status register.
		 */
1:		LDC1	($f0,(SC_FPREGS+0)(a0))
		LDC1	($f2,(SC_FPREGS+16)(a0))
		LDC1	($f4,(SC_FPREGS+32)(a0))
		LDC1	($f6,(SC_FPREGS+48)(a0))
		LDC1	($f8,(SC_FPREGS+64)(a0))
		LDC1	($f10,(SC_FPREGS+80)(a0))
		LDC1	($f12,(SC_FPREGS+96)(a0))
		LDC1	($f14,(SC_FPREGS+112)(a0))
		LDC1	($f16,(SC_FPREGS+128)(a0))
		LDC1	($f18,(SC_FPREGS+144)(a0))
		LDC1	($f20,(SC_FPREGS+160)(a0))
		LDC1	($f22,(SC_FPREGS+176)(a0))
		LDC1	($f24,(SC_FPREGS+192)(a0))
		LDC1	($f26,(SC_FPREGS+208)(a0))
		LDC1	($f28,(SC_FPREGS+224)(a0))
		LDC1	($f30,(SC_FPREGS+240)(a0))
		jr	ra
		.set	nomacro
		ctc1	t0,fcr31			# delay slot
		.set	macro

2:		jr	ra
		.set	nomacro
		nop					# delay slot
		.set	macro
		END(mips3_restore_fp_context)
		.set	reorder
