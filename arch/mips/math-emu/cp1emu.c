/*
 * cp1emu.c: a MIPS coprocessor 1 (fpu) instruction emulator
 * 
 * MIPS floating point support
 * Copyright (C) 1994-2000 Algorithmics Ltd.  All rights reserved.
 * http://www.algor.co.uk
 *
 * Kevin D. Kissell, kevink@mips.com and Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000  MIPS Technologies, Inc.
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * A complete emulator for MIPS coprocessor 1 instructions.  This is
 * required for #float(switch) or #float(trap), where it catches all
 * COP1 instructions via the "CoProcessor Unusable" exception.  
 *
 * More surprisingly it is also required for #float(ieee), to help out
 * the hardware fpu at the boundaries of the IEEE-754 representation
 * (denormalised values, infinities, underflow, etc).  It is made
 * quite nasty because emulation of some non-COP1 instructions is
 * required, e.g. in branch delay slots.
 * 
 * Note if you know that you won't have an fpu, then you'll get much 
 * better performance by compiling with -msoft-float!
 */
#include <linux/compiler.h>
#include <linux/mm.h>
#include <linux/signal.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>

#include <asm/asm.h>
#include <asm/branch.h>
#include <asm/bootinfo.h>
#include <asm/byteorder.h>
#include <asm/cpu.h>
#include <asm/inst.h>
#include <asm/uaccess.h>
#include <asm/processor.h>
#include <asm/mipsregs.h>
#include <asm/system.h>
#include <asm/pgtable.h>

#include <asm/fpu_emulator.h>

#include "ieee754.h"

/* Strap kernel emulator for full MIPS IV emulation */

#ifdef __mips
#undef __mips
#endif
#define __mips 4

typedef void *vaddr_t;

/* Function which emulates the instruction in a branch delay slot. */

static int mips_dsemul(struct pt_regs *, mips_instruction, unsigned long);

/* Function which emulates a floating point instruction. */

static int fpu_emu(struct pt_regs *, struct mips_fpu_soft_struct *,
	 mips_instruction);

#if __mips >= 4 && __mips != 32
static int fpux_emu(struct pt_regs *,
		    struct mips_fpu_soft_struct *, mips_instruction);
#endif

/* Further private data for which no space exists in mips_fpu_soft_struct */

struct mips_fpu_emulator_private fpuemuprivate;

/* Control registers */

#define FPCREG_RID	0	/* $0  = revision id */
#define FPCREG_CSR	31	/* $31 = csr */

/* Convert Mips rounding mode (0..3) to IEEE library modes. */
static const unsigned char ieee_rm[4] = {
	IEEE754_RN, IEEE754_RZ, IEEE754_RU, IEEE754_RD
};

#if __mips >= 4
/* convert condition code register number to csr bit */
static const unsigned int fpucondbit[8] = {
	FPU_CSR_COND0,
	FPU_CSR_COND1,
	FPU_CSR_COND2,
	FPU_CSR_COND3,
	FPU_CSR_COND4,
	FPU_CSR_COND5,
	FPU_CSR_COND6,
	FPU_CSR_COND7
};
#endif



/* 
 * Redundant with logic already in kernel/branch.c,
 * embedded in compute_return_epc.  At some point,
 * a single subroutine should be used across both
 * modules.
 */
static int isBranchInstr(mips_instruction * i)
{
	switch (MIPSInst_OPCODE(*i)) {
	case spec_op:
		switch (MIPSInst_FUNC(*i)) {
		case jalr_op:
		case jr_op:
			return 1;
		}
		break;

	case bcond_op:
		switch (MIPSInst_RT(*i)) {
		case bltz_op:
		case bgez_op:
		case bltzl_op:
		case bgezl_op:
		case bltzal_op:
		case bgezal_op:
		case bltzall_op:
		case bgezall_op:
			return 1;
		}
		break;

	case j_op:
	case jal_op:
	case jalx_op:
	case beq_op:
	case bne_op:
	case blez_op:
	case bgtz_op:
	case beql_op:
	case bnel_op:
	case blezl_op:
	case bgtzl_op:
		return 1;

	case cop0_op:
	case cop1_op:
	case cop2_op:
	case cop1x_op:
		if (MIPSInst_RS(*i) == bc_op)
			return 1;
		break;
	}

	return 0;
}

#define REG_TO_VA (vaddr_t)
#define VA_TO_REG (unsigned long)

/*
 * In the Linux kernel, we support selection of FPR format on the
 * basis of the Status.FR bit.  This does imply that, if a full 32
 * FPRs are desired, there needs to be a flip-flop that can be written
 * to one at that bit position.  In any case, normal MIPS ABI uses
 * only the even FPRs (Status.FR = 0).
 */

#define CP0_STATUS_FR_SUPPORT

/*
 * Emulate the single floating point instruction pointed at by EPC.
 * Two instructions if the instruction is in a branch delay slot.
 */

static int cop1Emulate(struct pt_regs *regs, struct mips_fpu_soft_struct *ctx)
{
	vaddr_t emulpc, contpc;
	mips_instruction ir;
	unsigned int cond;
	int err = 0;

	err = get_user(ir, (mips_instruction *) regs->cp0_epc);
	if (err) {
		fpuemuprivate.stats.errors++;
		return SIGBUS;
	}

	/* XXX NEC Vr54xx bug workaround */
	if ((regs->cp0_cause & CAUSEF_BD) && !isBranchInstr(&ir))
		regs->cp0_cause &= ~CAUSEF_BD;

	if (regs->cp0_cause & CAUSEF_BD) {
		/*
		 * The instruction to be emulated is in a branch delay slot
		 * which means that we have to  emulate the branch instruction
		 * BEFORE we do the cop1 instruction. 
		 *
		 * This branch could be a COP1 branch, but in that case we
		 * would have had a trap for that instruction, and would not
		 * come through this route.
		 *
		 * Linux MIPS branch emulator operates on context, updating the
		 * cp0_epc.
		 */
		emulpc = REG_TO_VA(regs->cp0_epc + 4);	/* Snapshot emulation target */

		if (__compute_return_epc(regs)) {
#ifdef CP1DBG
			printk("failed to emulate branch at %p\n",
				    REG_TO_VA(regs->cp0_epc));
#endif
			return SIGILL;
		}
		err = get_user(ir, (mips_instruction *) emulpc);
		if (err) {
			fpuemuprivate.stats.errors++;
			return SIGBUS;
		}
		/* __computer_return_epc() will have updated cp0_epc */
		contpc = REG_TO_VA regs->cp0_epc;
		/* In order not to confuse ptrace() et al, tweak context */
		regs->cp0_epc = VA_TO_REG emulpc - 4;
	} else {
		emulpc = REG_TO_VA regs->cp0_epc;
		contpc = REG_TO_VA regs->cp0_epc + 4;
	}

emul:
	fpuemuprivate.stats.emulated++;
	switch (MIPSInst_OPCODE(ir)) {
#ifdef CP0_STATUS_FR_SUPPORT
		/* R4000+ 64-bit fpu registers */
#ifndef SINGLE_ONLY_FPU
	case ldc1_op:
		{
			u64 *va = REG_TO_VA(regs->regs[MIPSInst_RS(ir)]) +
			          MIPSInst_SIMM(ir);
			int ft = MIPSInst_RT(ir);

			if (!(regs->cp0_status & ST0_FR))
				ft &= ~1;
			err = get_user(ctx->regs[ft], va);
			fpuemuprivate.stats.loads++;
			if (err) {
				fpuemuprivate.stats.errors++;
				return SIGBUS;
			}
		}
		break;

	case sdc1_op:
		{
			fpureg_t *va = REG_TO_VA(regs->regs[MIPSInst_RS(ir)]) +
			               MIPSInst_SIMM(ir);
			int ft = MIPSInst_RT(ir);

			if (!(regs->cp0_status & ST0_FR))
				ft &= ~1;
			fpuemuprivate.stats.stores++;
			if (put_user(ctx->regs[ft], va)) {
				fpuemuprivate.stats.errors++;
				return SIGBUS;
			}
		}
		break;
#endif

	case lwc1_op:
		{
			u32 *va = REG_TO_VA(regs->regs[MIPSInst_RS(ir)]) +
			               MIPSInst_SIMM(ir);
			int ft = MIPSInst_RT(ir);
			u32 val;

			fpuemuprivate.stats.loads++;
			err = get_user(val, va);
			if (err) {
				fpuemuprivate.stats.errors++;
				return SIGBUS;
			}
			if (regs->cp0_status & ST0_FR) {
				/* load whole register */
				ctx->regs[ft] = (s64) val;
			} else if (ft & 1) {
				/* load to m.s. 32 bits */
#ifdef SINGLE_ONLY_FPU
				/* illegal register in single-float mode */
				return SIGILL;
#else
				ctx->regs[(ft & ~1)] &= 0xffffffff;
				ctx->regs[(ft & ~1)] |= (fpureg_t) val << 32;
#endif
			} else {
				/* load to l.s. 32 bits */
				ctx->regs[ft] &= ~0xffffffffLL;
				ctx->regs[ft] |= val;
			}
		}
		break;

	case swc1_op:
		{
			u32 *va = REG_TO_VA(regs->regs[MIPSInst_RS(ir)]) +
			          MIPSInst_SIMM(ir);
			unsigned int val;
			int ft = MIPSInst_RT(ir);

			fpuemuprivate.stats.stores++;
			if (regs->cp0_status & ST0_FR) {
				/* store whole register */
				val = ctx->regs[ft];
			} else if (ft & 1) {
#ifdef SINGLE_ONLY_FPU
				/* illegal register in single-float mode */
				return SIGILL;
#else
				/* store from m.s. 32 bits */
				val = ctx->regs[(ft & ~1)] >> 32;
#endif
			} else {
				/* store from l.s. 32 bits */
				val = ctx->regs[ft];
			}
			if (put_user(val, va)) {
				fpuemuprivate.stats.errors++;
				return SIGBUS;
			}
		}
		break;
#else				/* old 32-bit fpu registers */
	case lwc1_op:
		{
			u32 *va = REG_TO_VA(regs->regs[MIPSInst_RS(ir)]) +
			          MIPSInst_SIMM(ir);
			err = get_user(ctx->regs[MIPSInst_RT(ir)], va);

			fpuemuprivate.stats.loads++;
			if (err) {
				fpuemuprivate.stats.errors++;
				return SIGBUS;
			}
		}
		break;

	case swc1_op:
		{
			u32 *va = REG_TO_VA(regs->regs[MIPSInst_RS(ir)]) +
			          MIPSInst_SIMM(ir);
			fpuemuprivate.stats.stores++;
			if (put_user(ctx->regs[MIPSInst_RT(ir)], va)) {
				fpuemuprivate.stats.errors++;
				return SIGBUS;
			}
		}
		break;
	case ldc1_op:
		{
			u32 *va = REG_TO_VA(regs->regs[MIPSInst_RS(ir)])
			    + MIPSInst_SIMM(ir);
			unsigned int rt = MIPSInst_RT(ir) & ~1;
			int errs = 0;
			fpuemuprivate.stats.loads++;
#if (defined(BYTE_ORDER) && BYTE_ORDER == BIG_ENDIAN) || defined(__MIPSEB__)
			err = get_user(ctx->regs[rt + 1], va + 0);
			err |= get_user(ctx->regs[rt + 0], va + 1);
#else
			err = get_user(ctx->regs[rt + 0], va + 0);
			err |= get_user(ctx->regs[rt + 1], va + 1);
#endif
			if (err)
				return SIGBUS;
		}
		break;

	case sdc1_op:
		{
			u32 *va = REG_TO_VA(regs->regs[MIPSInst_RS(ir)]) +
			          MIPSInst_SIMM(ir);
			unsigned int rt = MIPSInst_RT(ir) & ~1;

			fpuemuprivate.stats.stores++;
#if (defined(BYTE_ORDER) && BYTE_ORDER == BIG_ENDIAN) || defined(__MIPSEB__)
			if (put_user(ctx->regs[rt + 1], va + 0)
			    || put_user(ctx->regs[rt + 0], va + 1))
				return SIGBUS;
#else
			if (put_user(ctx->regs[rt + 0], va + 0)
			    || put_user(ctx->regs[rt + 1], va + 1))
				return SIGBUS;
#endif
		}
		break;
#endif

	case cop1_op:
		switch (MIPSInst_RS(ir)) {

#ifdef CP0_STATUS_FR_SUPPORT
#if __mips64 && !defined(SINGLE_ONLY_FPU)
		case dmfc_op:
			/* copregister fs -> gpr[rt] */
			if (MIPSInst_RT(ir) != 0) {
				int fs = MIPSInst_RD(ir);
				if (!(regs->cp0_status & ST0_FR))
					fs &= ~1;
				regs->regs[MIPSInst_RT(ir)] = ctx->regs[fs];
			}
			break;

		case dmtc_op: {
			/* copregister fs <- rt */
				fpureg_t value;
				int fs = MIPSInst_RD(ir);
				if (!(regs->cp0_status & ST0_FR))
					fs &= ~1;
				value =
				    (MIPSInst_RT(ir) ==
				     0) ? 0 : regs->regs[MIPSInst_RT(ir)];
				ctx->regs[fs] = value;
			break;
		}
#endif

		case mfc_op:
			/* copregister rd -> gpr[rt] */
			if (MIPSInst_RT(ir) != 0) {
				/* default value from l.s. 32 bits */
				int value = ctx->regs[MIPSInst_RD(ir)];
				if (MIPSInst_RD(ir) & 1) {
#ifdef SINGLE_ONLY_FPU
					/* illegal register in single-float mode */
					return SIGILL;
#else
					if (!(regs->cp0_status & ST0_FR)) {
						/* move from m.s. 32 bits */
						value =
						    ctx->
						    regs[MIPSInst_RD(ir) &
							 ~1] >> 32;
					}
#endif
				}
				regs->regs[MIPSInst_RT(ir)] = value;
			}
			break;

		case mtc_op:
			/* copregister rd <- rt */
			{
				fpureg_t value;
				if (MIPSInst_RT(ir) == 0)
					value = 0;
				else
					value =
					    (unsigned int) regs->
					    regs[MIPSInst_RT(ir)];
				if (MIPSInst_RD(ir) & 1) {
#ifdef SINGLE_ONLY_FPU
					/* illegal register in single-float mode */
					return SIGILL;
#else
					if (!(regs->cp0_status & ST0_FR)) {
						/* move to m.s. 32 bits */
						ctx->
						    regs[
							 (MIPSInst_RD(ir) &
							  ~1)] &=
						    0xffffffff;
						ctx->
						    regs[
							 (MIPSInst_RD(ir) &
							  ~1)] |=
						    value << 32;
						break;
					}
#endif
				}
				/* move to l.s. 32 bits */
				ctx->regs[MIPSInst_RD(ir)] &=
				    ~0xffffffffLL;
				ctx->regs[MIPSInst_RD(ir)] |= value;
			}
			break;
#else

		case mfc_op:
			/* copregister rd -> gpr[rt] */
			if (MIPSInst_RT(ir) != 0) {
				unsigned value =
				    ctx->regs[MIPSInst_RD(ir)];
				regs->regs[MIPSInst_RT(ir)] = value;
			}
			break;

		case mtc_op:
			/* copregister rd <- rt */
			{
				unsigned value;
				value =
				    (MIPSInst_RT(ir) ==
				     0) ? 0 : regs->regs[MIPSInst_RT(ir)];
				ctx->regs[MIPSInst_RD(ir)] = value;
			}
			break;
#endif

		case cfc_op:
			/* cop control register rd -> gpr[rt] */
			{
				unsigned value;

				if (MIPSInst_RD(ir) == FPCREG_CSR) {
					value = ctx->sr;
#ifdef CSRTRACE
					printk
					    ("%p gpr[%d]<-csr=%08x\n",
					     REG_TO_VA(regs->cp0_epc),
					     MIPSInst_RT(ir), value);
#endif
				} else if (MIPSInst_RD(ir) == FPCREG_RID)
					value = 0;
				else
					value = 0;
				if (MIPSInst_RT(ir))
					regs->regs[MIPSInst_RT(ir)] = value;
			}
			break;

		case ctc_op:
			/* copregister rd <- rt */
			{
				unsigned value;

				if (MIPSInst_RT(ir) == 0)
					value = 0;
				else
					value = regs->regs[MIPSInst_RT(ir)];

				/* we only have one writable control reg
				 */
				if (MIPSInst_RD(ir) == FPCREG_CSR) {
#ifdef CSRTRACE
					printk
					    ("%p gpr[%d]->csr=%08x\n",
					     REG_TO_VA(regs->cp0_epc),
					     MIPSInst_RT(ir), value);
#endif
					ctx->sr = value;
					/* copy new rounding mode and
					   flush bit to ieee library state! */
					ieee754_csr.nod =
					    (ctx->sr & 0x1000000) != 0;
					ieee754_csr.rm =
					    ieee_rm[value & 0x3];
				}
			}
			break;

		case bc_op: {
			int likely = 0;

			if (regs->cp0_cause & CAUSEF_BD)
				return SIGILL;

#if __mips >= 4
			cond = ctx-> sr & fpucondbit[MIPSInst_RT(ir) >> 2];
#else
			cond = ctx->sr & FPU_CSR_COND;
#endif
			switch (MIPSInst_RT(ir) & 3) {
			case bcfl_op:
				likely = 1;
			case bcf_op:
				cond = !cond;
				break;
			case bctl_op:
				likely = 1;
			case bct_op:
				break;
			default:
				/* thats an illegal instruction */
				return SIGILL;
			}

			regs->cp0_cause |= CAUSEF_BD;
			if (cond) {
				/* branch taken: emulate dslot instruction */
				regs->cp0_epc += 4;
				contpc = REG_TO_VA regs->cp0_epc +
				                   (MIPSInst_SIMM(ir) << 2);

				err = get_user(ir,
					(mips_instruction *)regs->cp0_epc);
				if (err) {
					fpuemuprivate.stats.errors++;
					return SIGBUS;
				}

				switch (MIPSInst_OPCODE(ir)) {
				case lwc1_op:
				case swc1_op:
#if (__mips >= 2 || __mips64) && !defined(SINGLE_ONLY_FPU)
				case ldc1_op:
				case sdc1_op:
#endif
				case cop1_op:
#if __mips >= 4 && __mips != 32
				case cop1x_op:
#endif
					/* its one of ours */
					goto emul;
#if __mips >= 4
				case spec_op:
					if (MIPSInst_FUNC(ir) == movc_op)
						goto emul;
					break;
#endif
				}

				/*
				 * Single step the non-cp1 instruction in the
				 * dslot
				 */
				return mips_dsemul(regs, ir, contpc);
			} else {
				/* branch not taken */
				if (likely) {
					/*
					 * branch likely nullifies dslot if not
					 * taken
					 */
					regs->cp0_epc += 4;
					contpc += 4;
					/*
					 * else continue & execute dslot as
					 * normal insn
					 */
				}
			}
			break;
		}

		default: {
			int sig;

			if (!(MIPSInst_RS(ir) & 0x10))
				return SIGILL;

			/* a real fpu computation instruction */
				if ((sig = fpu_emu(regs, ctx, ir)))
					return sig;
			}
		}
		break;

#if __mips >= 4 && __mips != 32
	case cop1x_op:
		{
			int sig;
			if ((sig = fpux_emu(regs, ctx, ir)))
				return sig;
		}
		break;
#endif

#if __mips >= 4
	case spec_op:
		if (MIPSInst_FUNC(ir) != movc_op)
			return SIGILL;
		cond = fpucondbit[MIPSInst_RT(ir) >> 2];
		if (((ctx->sr & cond) != 0) != ((MIPSInst_RT(ir) & 1) != 0))
			return 0;
		regs->regs[MIPSInst_RD(ir)] = regs->regs[MIPSInst_RS(ir)];
		break;
#endif

	default:
		return SIGILL;
	}

	/* we did it !! */
	regs->cp0_epc = VA_TO_REG(contpc);
	regs->cp0_cause &= ~CAUSEF_BD;
	return 0;
}

/*
 * Emulate the arbritrary instruction ir at xcp->cp0_epc.  Required when
 * we have to emulate the instruction in a COP1 branch delay slot.  Do
 * not change cp0_epc due to the instruction
 *
 * According to the spec:
 * 1) it shouldnt be a branch :-)
 * 2) it can be a COP instruction :-(
 * 3) if we are tring to run a protected memory space we must take
 *    special care on memory access instructions :-(
 */

/*
 * "Trampoline" return routine to catch exception following
 *  execution of delay-slot instruction execution.
 */

/* Instruction inserted following delay slot instruction to force trap */
#define AdELOAD 0x8c000001	/* lw $0,1($0) */

/* Instruction inserted following the AdELOAD to further tag the sequence */
#define BD_COOKIE 0x0000bd36	/* tne $0,$0 with baggage */

struct emuframe {
	mips_instruction	emul;
	mips_instruction	adel;
	mips_instruction	cookie;
	unsigned long	epc;
};

int do_dsemulret(struct pt_regs *xcp)
{
	struct emuframe *fr;
	unsigned long epc;
	u32 insn, cookie;
	int err = 0;

	fr = (struct emuframe *) (xcp->cp0_epc - sizeof(mips_instruction));

	/* 
	 * If we can't even access the area, something is very wrong, but we'll
	 * leave that to the default handling
	 */
	if (verify_area(VERIFY_READ, fr, sizeof(struct emuframe)))
		return 0;

	/*
	 * Do some sanity checking on the stackframe:
	 *
	 *  - Is the instruction pointed to by the EPC an AdELOAD?
	 *  - Is the following memory word the BD_COOKIE?
	 */
	err = __get_user(insn, &fr->adel);
	err |= __get_user(cookie, &fr->cookie);

	if (unlikely(err || (insn != AdELOAD) || (cookie != BD_COOKIE))) {
		fpuemuprivate.stats.errors++;

		return 0;
	}

	/* 
	 * At this point, we are satisfied that it's a BD emulation trap.  Yes,
	 * a user might have deliberately put two malformed and useless
	 * instructions in a row in his program, in which case he's in for a
	 * nasty surprise - the next instruction will be treated as a
	 * continuation address!  Alas, this seems to be the only way that we
	 * can handle signals, recursion, and longjmps() in the context of
	 * emulating the branch delay instruction.
	 */

#ifdef DSEMUL_TRACE
	printk("desemulret\n");
#endif
	if (__get_user(epc, &fr->epc)) {		/* Saved EPC */
		/* This is not a good situation to be in */
		force_sig(SIGBUS, current);

		return 1;
	}

	/* Set EPC to return to post-branch instruction */
	xcp->cp0_epc = epc;

	return 1;
}

static int mips_dsemul(struct pt_regs *regs, mips_instruction ir,
	unsigned long cpc)
{
	extern asmlinkage void handle_dsemulret(void);
	mips_instruction *dsemul_insns;
	struct emuframe *fr;
	int err;

	if (ir == 0) {		/* a nop is easy */
		regs->cp0_epc = cpc;
		regs->cp0_cause &= ~CAUSEF_BD;
		return 0;
	}
#ifdef DSEMUL_TRACE
	printk("desemul %lx %lx\n", regs->cp0_epc, cpc);

#endif
 
	/* 
	 * The strategy is to push the instruction onto the user stack 
	 * and put a trap after it which we can catch and jump to 
	 * the required address any alternative apart from full 
	 * instruction emulation!!.
	 *
	 * Algorithmics used a system call instruction, and
	 * borrowed that vector.  MIPS/Linux version is a bit
	 * more heavyweight in the interests of portability and
	 * multiprocessor support.  We flag the thread for special
	 * handling in the unaligned access handler and force an
	 * address error excpetion.
	 */

	/* Ensure that the two instructions are in the same cache line */
	dsemul_insns = (mips_instruction *) (regs->regs[29] & ~0xf);
	dsemul_insns -= 4;	/* Retain 16-byte alignment */
	fr = (struct emuframe *) dsemul_insns;

	/* Verify that the stack pointer is not competely insane */
	if (unlikely(verify_area(VERIFY_WRITE, fr, sizeof(struct emuframe))))
		return SIGBUS;

	err = __put_user(ir, &dsemul_insns[0]);
	err |= __put_user((mips_instruction)AdELOAD, &fr->adel);
	err |= __put_user((mips_instruction)BD_COOKIE, &fr->cookie);
	err |= __put_user(cpc, &fr->epc);

	if (unlikely(err)) {
		fpuemuprivate.stats.errors++;
		return SIGBUS;
	}

	regs->cp0_epc = VA_TO_REG & dsemul_insns[0];

	flush_cache_sigtramp((unsigned long)&fr->adel);

	return SIGILL;		/* force out of emulation loop */
}

/* 
 * Conversion table from MIPS compare ops 48-63
 * cond = ieee754dp_cmp(x,y,IEEE754_UN);
 */
static const unsigned char cmptab[8] = {
	0,					/* cmp_0 (sig) cmp_sf */
	IEEE754_CUN,				/* cmp_un (sig) cmp_ngle */
	IEEE754_CEQ,				/* cmp_eq (sig) cmp_seq */
	IEEE754_CEQ | IEEE754_CUN,		/* cmp_ueq (sig) cmp_ngl  */
	IEEE754_CLT,				/* cmp_olt (sig) cmp_lt */
	IEEE754_CLT | IEEE754_CUN,		/* cmp_ult (sig) cmp_nge */
	IEEE754_CLT | IEEE754_CEQ,		/* cmp_ole (sig) cmp_le */
	IEEE754_CLT | IEEE754_CEQ | IEEE754_CUN, /* cmp_ule (sig) cmp_ngt */
};

#define SIFROMREG(si,x)	((si) = ctx->regs[x])
#define SITOREG(si,x)	(ctx->regs[x] = (int)(si))

#if __mips64 && !defined(SINGLE_ONLY_FPU)
#define DIFROMREG(di,x)	((di) = ctx->regs[x])
#define DITOREG(di,x)	(ctx->regs[x] = (di))
#endif

#define SPFROMREG(sp,x)	((sp).bits = ctx->regs[x])
#define SPTOREG(sp,x)	(ctx->regs[x] = (sp).bits)

#ifdef CP0_STATUS_FR_SUPPORT
#define DPFROMREG(dp,x)	((dp).bits = \
	ctx->regs[(xcp->cp0_status & ST0_FR) ? x : (x & ~1)])
#define DPTOREG(dp,x)	(ctx->regs[(xcp->cp0_status & ST0_FR) ? x : (x & ~1)] \
	= (dp).bits)
#else
/* Beware: MIPS COP1 doubles are always little_word endian in registers */
#define DPFROMREG(dp,x)	\
	((dp).bits = ((u64)ctx->regs[(x)+1] << 32) | ctx->regs[x])
#define DPTOREG(dp,x) \
	(ctx->regs[x] = (dp).bits, ctx->regs[(x)+1] = (dp).bits >> 32)
#endif

#if __mips >= 4 && __mips != 32

/*
 * Additional MIPS4 instructions
 */

#define DEF3OP(name, p, f1, f2, f3) \
static ieee754##p fpemu_##p##_##name (ieee754##p r, ieee754##p s, \
    ieee754##p t) \
{ \
    struct ieee754_csr ieee754_csr_save; \
    s = f1 (s, t); \
    ieee754_csr_save = ieee754_csr; \
    s = f2 (s, r); \
    ieee754_csr_save.cx |= ieee754_csr.cx; \
    ieee754_csr_save.sx |= ieee754_csr.sx; \
    s = f3 (s); \
    ieee754_csr.cx |= ieee754_csr_save.cx; \
    ieee754_csr.sx |= ieee754_csr_save.sx; \
    return s; \
}    

static ieee754dp fpemu_dp_recip(ieee754dp d)
{
	return ieee754dp_div(ieee754dp_one(0), d);
}

static ieee754dp fpemu_dp_rsqrt(ieee754dp d)
{
	return ieee754dp_div(ieee754dp_one(0), ieee754dp_sqrt(d));
}

static ieee754sp fpemu_sp_recip(ieee754sp s)
{
	return ieee754sp_div(ieee754sp_one(0), s);
}

static ieee754sp fpemu_sp_rsqrt(ieee754sp s)
{
	return ieee754sp_div(ieee754sp_one(0), ieee754sp_sqrt(s));
}

DEF3OP(madd, sp, ieee754sp_mul, ieee754sp_add, );
DEF3OP(msub, sp, ieee754sp_mul, ieee754sp_sub, );
DEF3OP(nmadd, sp, ieee754sp_mul, ieee754sp_add, ieee754sp_neg);
DEF3OP(nmsub, sp, ieee754sp_mul, ieee754sp_sub, ieee754sp_neg);
DEF3OP(madd, dp, ieee754dp_mul, ieee754dp_add, );
DEF3OP(msub, dp, ieee754dp_mul, ieee754dp_sub, );
DEF3OP(nmadd, dp, ieee754dp_mul, ieee754dp_add, ieee754dp_neg);
DEF3OP(nmsub, dp, ieee754dp_mul, ieee754dp_sub, ieee754dp_neg);

static int fpux_emu(struct pt_regs *xcp, struct mips_fpu_soft_struct *ctx,
                    mips_instruction ir)
{
	unsigned rcsr = 0;	/* resulting csr */

	fpuemuprivate.stats.cp1xops++;

	switch (MIPSInst_FMA_FFMT(ir)) {
	case s_fmt:		/* 0 */
		{
			ieee754sp(*handler) (ieee754sp, ieee754sp,
					     ieee754sp);
			ieee754sp fd, fr, fs, ft;

			switch (MIPSInst_FUNC(ir)) {
			case lwxc1_op:
				{
					u32 *va = REG_TO_VA(
						xcp->regs[MIPSInst_FR(ir)] +
						xcp->regs[MIPSInst_FT(ir)]);
					fpureg_t val;

					if (get_user(val, va)) {
						fpuemuprivate.stats.errors++;
						return SIGBUS;
					}
					if (xcp->cp0_status & ST0_FR) {
						/* load whole register */
						ctx->regs[MIPSInst_FD(ir)] =
						    val;
					} else if (MIPSInst_FD(ir) & 1) {
						/* load to m.s. 32 bits */
#if defined(SINGLE_ONLY_FPU)
						/* illegal register in single-float mode */
						return SIGILL;
#else
						ctx->
						    regs[
							 (MIPSInst_FD(ir) &
							  ~1)] &=
						    0xffffffff;
						ctx->
						    regs[
							 (MIPSInst_FD(ir) &
							  ~1)] |=
						    val << 32;
#endif
					} else {
						/* load to l.s. 32 bits */
						ctx->
						    regs[MIPSInst_FD(ir)]
						    &= ~0xffffffffLL;
						ctx->
						    regs[MIPSInst_FD(ir)]
						    |= val;
					}
				}
				break;

			case swxc1_op:
				{
					u32 *va = REG_TO_VA(
						xcp->regs[MIPSInst_FR(ir)] +
						xcp->regs[MIPSInst_FT(ir)]);
					unsigned int val;

					if (xcp->cp0_status & ST0_FR) {
						/* store whole register */
						val = ctx->regs[MIPSInst_FS(ir)];
					} else if (MIPSInst_FS(ir) & 1) {
#if defined(SINGLE_ONLY_FPU)
						/* illegal register in single-float mode */
						return SIGILL;
#else
						/* store from m.s. 32 bits */
						val = ctx->regs[(MIPSInst_FS(ir) & ~1)] >> 32;
#endif
					} else {
						/* store from l.s. 32 bits */
						val =
						    ctx->
						    regs[MIPSInst_FS(ir)];
					}
					if (put_user(val, va)) {
						fpuemuprivate.stats.errors++;
						return SIGBUS;
					}
				}
				break;

			case madd_s_op:
				handler = fpemu_sp_madd;
				goto scoptop;
			case msub_s_op:
				handler = fpemu_sp_msub;
				goto scoptop;
			case nmadd_s_op:
				handler = fpemu_sp_nmadd;
				goto scoptop;
			case nmsub_s_op:
				handler = fpemu_sp_nmsub;
				goto scoptop;

			      scoptop:
				SPFROMREG(fr, MIPSInst_FR(ir));
				SPFROMREG(fs, MIPSInst_FS(ir));
				SPFROMREG(ft, MIPSInst_FT(ir));
				fd = (*handler) (fr, fs, ft);
				SPTOREG(fd, MIPSInst_FD(ir));

			      copcsr:
				if (ieee754_cxtest(IEEE754_INEXACT))
					rcsr |=
					    FPU_CSR_INE_X | FPU_CSR_INE_S;
				if (ieee754_cxtest(IEEE754_UNDERFLOW))
					rcsr |=
					    FPU_CSR_UDF_X | FPU_CSR_UDF_S;
				if (ieee754_cxtest(IEEE754_OVERFLOW))
					rcsr |=
					    FPU_CSR_OVF_X | FPU_CSR_OVF_S;
				if (ieee754_cxtest
				    (IEEE754_INVALID_OPERATION)) rcsr |=
					    FPU_CSR_INV_X | FPU_CSR_INV_S;

				ctx->sr =
				    (ctx->sr & ~FPU_CSR_ALL_X) | rcsr;
				if (ieee754_csr.nod)
				    ctx->sr |= 0x1000000;
				if ((ctx->sr >> 5) & ctx->
				    sr & FPU_CSR_ALL_E) {
		/*printk ("SIGFPE: fpu csr = %08x\n",ctx->sr); */
					return SIGFPE;
				}

				break;

			default:
				return SIGILL;
			}
		}
		break;

#if !defined(SINGLE_ONLY_FPU)
	case d_fmt:		/* 1 */
		{
			ieee754dp(*handler) (ieee754dp, ieee754dp,
					     ieee754dp);
			ieee754dp fd, fr, fs, ft;

			switch (MIPSInst_FUNC(ir)) {
			case ldxc1_op:
				{
					u64 *va = REG_TO_VA(
						xcp->regs[MIPSInst_FR(ir)] +
						xcp->regs[MIPSInst_FT(ir)]);
					u64 val;

					if (get_user(val, va)) {
						fpuemuprivate.stats.errors++;
						return SIGBUS;
					}
					ctx->regs[MIPSInst_FD(ir)] = val;
				}
				break;

			case sdxc1_op:
				{
					u64 *va = REG_TO_VA(
						xcp->regs[MIPSInst_FR(ir)] +
						xcp->regs[MIPSInst_FT(ir)]);
					u64 val;

					val = ctx->regs[MIPSInst_FS(ir)];
					if (put_user(val, va)) {
						fpuemuprivate.stats.errors++;
						return SIGBUS;
					}
				}
				break;

			case madd_d_op:
				handler = fpemu_dp_madd;
				goto dcoptop;

			case msub_d_op:
				handler = fpemu_dp_msub;
				goto dcoptop;

			case nmadd_d_op:
				handler = fpemu_dp_nmadd;
				goto dcoptop;

			case nmsub_d_op:
				handler = fpemu_dp_nmsub;
				goto dcoptop;

			      dcoptop:
				DPFROMREG(fr, MIPSInst_FR(ir));
				DPFROMREG(fs, MIPSInst_FS(ir));
				DPFROMREG(ft, MIPSInst_FT(ir));
				fd = (*handler) (fr, fs, ft);
				DPTOREG(fd, MIPSInst_FD(ir));
				goto copcsr;

			default:
				return SIGILL;
			}
		}
		break;
#endif

	case 0x7:		/* 7 */
		{
			if (MIPSInst_FUNC(ir) != pfetch_op) {
				return SIGILL;
			}
			/* ignore prefx operation */
		}
		break;

	default:
		return SIGILL;
	}

	return 0;
}
#endif



/*
 * Emulate a single COP1 arithmetic instruction.
 */
static int fpu_emu(struct pt_regs *xcp, struct mips_fpu_soft_struct *ctx,
                   mips_instruction ir)
{
	int rfmt;		/* resulting format */
	unsigned rcsr = 0;	/* resulting csr */
	unsigned cond;
	union {
		ieee754dp d;
		ieee754sp s;
		int w;
#if __mips64
		long long l;
#endif
	} rv;			/* resulting value */

	fpuemuprivate.stats.cp1ops++;
	switch (rfmt = (MIPSInst_FFMT(ir) & 0xf)) {
	case s_fmt: {		/* 0 */
		ieee754sp(*handler) ();

		switch (MIPSInst_FUNC(ir)) {
			/* binary ops */
		case fadd_op:
			handler = ieee754sp_add;
			goto scopbop;
		case fsub_op:
			handler = ieee754sp_sub;
			goto scopbop;
		case fmul_op:
			handler = ieee754sp_mul;
			goto scopbop;
		case fdiv_op:
			handler = ieee754sp_div;
			goto scopbop;

			/* unary  ops */
#if __mips >= 2 || __mips64
		case fsqrt_op:
			handler = ieee754sp_sqrt;
			goto scopuop;
#endif
#if __mips >= 4 && __mips != 32
		case frsqrt_op:
			handler = fpemu_sp_rsqrt;
			goto scopuop;
		case frecip_op:
			handler = fpemu_sp_recip;
			goto scopuop;
#endif
#if __mips >= 4
		case fmovc_op:
			cond = fpucondbit[MIPSInst_FT(ir) >> 2];
			if (((ctx->sr & cond) != 0) !=
			    ((MIPSInst_FT(ir) & 1) != 0))
				return 0;
			SPFROMREG(rv.s, MIPSInst_FS(ir));
			break;
		case fmovz_op:
			if (xcp->regs[MIPSInst_FT(ir)] != 0)
				return 0;
			SPFROMREG(rv.s, MIPSInst_FS(ir));
			break;
		case fmovn_op:
			if (xcp->regs[MIPSInst_FT(ir)] == 0)
				return 0;
			SPFROMREG(rv.s, MIPSInst_FS(ir));
			break;
#endif
		case fabs_op:
			handler = ieee754sp_abs;
			goto scopuop;
		case fneg_op:
			handler = ieee754sp_neg;
			goto scopuop;
		case fmov_op:
			/* an easy one */
			SPFROMREG(rv.s, MIPSInst_FS(ir));
			goto copcsr;

			/* binary op on handler */
scopbop:
			{
				ieee754sp fs, ft;

				SPFROMREG(fs, MIPSInst_FS(ir));
				SPFROMREG(ft, MIPSInst_FT(ir));

				rv.s = (*handler) (fs, ft);
				goto copcsr;
			}
scopuop:
			{
				ieee754sp fs;

				SPFROMREG(fs, MIPSInst_FS(ir));
				rv.s = (*handler) (fs);
				goto copcsr;
			}
copcsr:
			if (ieee754_cxtest(IEEE754_INEXACT))
				rcsr |= FPU_CSR_INE_X | FPU_CSR_INE_S;
			if (ieee754_cxtest(IEEE754_UNDERFLOW))
				rcsr |= FPU_CSR_UDF_X | FPU_CSR_UDF_S;
			if (ieee754_cxtest(IEEE754_OVERFLOW))
				rcsr |= FPU_CSR_OVF_X | FPU_CSR_OVF_S;
			if (ieee754_cxtest(IEEE754_ZERO_DIVIDE))
				rcsr |= FPU_CSR_DIV_X | FPU_CSR_DIV_S;
			if (ieee754_cxtest
				(IEEE754_INVALID_OPERATION)) rcsr |=
					    FPU_CSR_INV_X | FPU_CSR_INV_S;
				break;

				/* unary conv ops */
		case fcvts_op:
			return SIGILL;	/* not defined */
		case fcvtd_op: {
#ifdef SINGLE_ONLY_FPU
			return SIGILL;	/* not defined */
#else
			ieee754sp fs;

			SPFROMREG(fs, MIPSInst_FS(ir));
			rv.d = ieee754dp_fsp(fs);
			rfmt = d_fmt;
			goto copcsr;
		}
#endif
		case fcvtw_op: {
			ieee754sp fs;

			SPFROMREG(fs, MIPSInst_FS(ir));
			rv.w = ieee754sp_tint(fs);
			rfmt = w_fmt;
			goto copcsr;
		}

#if __mips >= 2 || __mips64
		case fround_op:
		case ftrunc_op:
		case fceil_op:
		case ffloor_op: {
			unsigned int oldrm = ieee754_csr.rm;
			ieee754sp fs;

			SPFROMREG(fs, MIPSInst_FS(ir));
			ieee754_csr.rm = ieee_rm[MIPSInst_FUNC(ir) & 0x3];
			rv.w = ieee754sp_tint(fs);
			ieee754_csr.rm = oldrm;
			rfmt = w_fmt;
			goto copcsr;
		}
#endif /* __mips >= 2 */

#if __mips64 && !defined(SINGLE_ONLY_FPU)
		case fcvtl_op: {
			ieee754sp fs;

			SPFROMREG(fs, MIPSInst_FS(ir));
			rv.l = ieee754sp_tlong(fs);
			rfmt = l_fmt;
			goto copcsr;
		}

		case froundl_op:
		case ftruncl_op:
		case fceill_op:
		case ffloorl_op: {
			unsigned int oldrm = ieee754_csr.rm;
			ieee754sp fs;

			SPFROMREG(fs, MIPSInst_FS(ir));
			ieee754_csr.rm = ieee_rm[MIPSInst_FUNC(ir) & 0x3];
			rv.l = ieee754sp_tlong(fs);
			ieee754_csr.rm = oldrm;
			rfmt = l_fmt;
			goto copcsr;
		}
#endif /* __mips64 && !fpu(single) */

		default:
			if (MIPSInst_FUNC(ir) >= fcmp_op) {
				unsigned cmpop = MIPSInst_FUNC(ir) - fcmp_op;
				ieee754sp fs, ft;

				SPFROMREG(fs, MIPSInst_FS(ir));
				SPFROMREG(ft, MIPSInst_FT(ir));
				rv.w = ieee754sp_cmp(fs, ft, cmptab[cmpop & 0x7]);
				rfmt = -1;
				if ((cmpop & 0x8) && ieee754_cxtest(IEEE754_INVALID_OPERATION))
					rcsr = FPU_CSR_INV_X | FPU_CSR_INV_S;
				else
					goto copcsr;

			} else {
				return SIGILL;
			}
			break;
		}
		break;
	}

#if !defined(SINGLE_ONLY_FPU)
	case d_fmt: {
		ieee754dp(*handler) ();

		switch (MIPSInst_FUNC(ir)) {
			/* binary ops */
		case fadd_op:
			handler = ieee754dp_add;
			goto dcopbop;
		case fsub_op:
			handler = ieee754dp_sub;
			goto dcopbop;
		case fmul_op:
			handler = ieee754dp_mul;
			goto dcopbop;
		case fdiv_op:
			handler = ieee754dp_div;
			goto dcopbop;

			/* unary  ops */
#if __mips >= 2 || __mips64
		case fsqrt_op:
			handler = ieee754dp_sqrt;
			goto dcopuop;
#endif
#if __mips >= 4 && __mips != 32
		case frsqrt_op:
			handler = fpemu_dp_rsqrt;
			goto dcopuop;
		case frecip_op:
			handler = fpemu_dp_recip;
			goto dcopuop;
#endif
#if __mips >= 4
		case fmovc_op:
			cond = fpucondbit[MIPSInst_FT(ir) >> 2];
			if (((ctx->sr & cond) != 0) != ((MIPSInst_FT(ir) & 1) != 0))
				return 0;
			DPFROMREG(rv.d, MIPSInst_FS(ir));
			break;
		case fmovz_op:
			if (xcp->regs[MIPSInst_FT(ir)] != 0)
				return 0;
			DPFROMREG(rv.d, MIPSInst_FS(ir));
			break;
		case fmovn_op:
			if (xcp->regs[MIPSInst_FT(ir)] == 0)
				return 0;
			DPFROMREG(rv.d, MIPSInst_FS(ir));
			break;
#endif
		case fabs_op:
			handler = ieee754dp_abs;
			goto dcopuop;

		case fneg_op:
			handler = ieee754dp_neg;
			goto dcopuop;

		case fmov_op:
			/* an easy one */
			DPFROMREG(rv.d, MIPSInst_FS(ir));
			goto copcsr;

			/* binary op on handler */
dcopbop:
			{
				ieee754dp fs, ft;

				DPFROMREG(fs, MIPSInst_FS(ir));
				DPFROMREG(ft, MIPSInst_FT(ir));

				rv.d = (*handler) (fs, ft);
				goto copcsr;
			}
dcopuop:
			{
				ieee754dp fs;

				DPFROMREG(fs, MIPSInst_FS(ir));
				rv.d = (*handler) (fs);
				goto copcsr;
			}

		/* unary conv ops */
		case fcvts_op: {
			ieee754dp fs;

			DPFROMREG(fs, MIPSInst_FS(ir));
			rv.s = ieee754sp_fdp(fs);
			rfmt = s_fmt;
			goto copcsr;
		}
		case fcvtd_op:
			return SIGILL;	/* not defined */

		case fcvtw_op: {
			ieee754dp fs;

			DPFROMREG(fs, MIPSInst_FS(ir));
			rv.w = ieee754dp_tint(fs);	/* wrong */
			rfmt = w_fmt;
			goto copcsr;
		}

#if __mips >= 2 || __mips64
		case fround_op:
		case ftrunc_op:
		case fceil_op:
		case ffloor_op: {
			unsigned int oldrm = ieee754_csr.rm;
			ieee754dp fs;

			DPFROMREG(fs, MIPSInst_FS(ir));
			ieee754_csr.rm = ieee_rm[MIPSInst_FUNC(ir) & 0x3];
			rv.w = ieee754dp_tint(fs);
			ieee754_csr.rm = oldrm;
			rfmt = w_fmt;
			goto copcsr;
		}
#endif

#if __mips64 && !defined(SINGLE_ONLY_FPU)
		case fcvtl_op: {
			ieee754dp fs;

			DPFROMREG(fs, MIPSInst_FS(ir));
			rv.l = ieee754dp_tlong(fs);
			rfmt = l_fmt;
			goto copcsr;
		}

		case froundl_op:
		case ftruncl_op:
		case fceill_op:
		case ffloorl_op: {
			unsigned int oldrm = ieee754_csr.rm;
			ieee754dp fs;

			DPFROMREG(fs, MIPSInst_FS(ir));
			ieee754_csr.rm = ieee_rm[MIPSInst_FUNC(ir) & 0x3];
			rv.l = ieee754dp_tlong(fs);
			ieee754_csr.rm = oldrm;
			rfmt = l_fmt;
			goto copcsr;
		}
#endif /* __mips >= 3 && !fpu(single) */

		default:
			if (MIPSInst_FUNC(ir) >= fcmp_op) {
				unsigned cmpop = MIPSInst_FUNC(ir) - fcmp_op;
				ieee754dp fs, ft;

				DPFROMREG(fs, MIPSInst_FS(ir));
				DPFROMREG(ft, MIPSInst_FT(ir));
				rv.w = ieee754dp_cmp(fs, ft, cmptab[cmpop & 0x7]);
				rfmt = -1;
				if ((cmpop & 0x8) && ieee754_cxtest (IEEE754_INVALID_OPERATION))
					rcsr = FPU_CSR_INV_X | FPU_CSR_INV_S;
				else
					goto copcsr;

			} else {
				return SIGILL;
			}
			break;
		}
		break;
	}
#endif /* !defined(SINGLE_ONLY_FPU) */

	case w_fmt: {
		switch (MIPSInst_FUNC(ir)) {
		case fcvts_op:
			/* convert word to single precision real */
			rv.s = ieee754sp_fint(ctx-> regs[MIPSInst_FS(ir)]);
			rfmt = s_fmt;
			goto copcsr;
#if !defined(SINGLE_ONLY_FPU)
		case fcvtd_op:
			/* convert word to double precision real */
			rv.d = ieee754dp_fint(ctx-> regs[MIPSInst_FS(ir)]);
			rfmt = d_fmt;
			goto copcsr;
#endif
		default:
			return SIGILL;
		}
		break;
	}

#if __mips64 && !defined(SINGLE_ONLY_FPU)
	case l_fmt: {
		switch (MIPSInst_FUNC(ir)) {
		case fcvts_op:
			/* convert long to single precision real */
			rv.s = ieee754sp_flong(ctx-> regs[MIPSInst_FS(ir)]);
			rfmt = s_fmt;
			goto copcsr;
		case fcvtd_op:
			/* convert long to double precision real */
			rv.d = ieee754dp_flong(ctx-> regs[MIPSInst_FS(ir)]);
			rfmt = d_fmt;
			goto copcsr;
		default:
			return SIGILL;
		}
		break;
	}
#endif

	default:
		return SIGILL;
	}

	/*
	 * Update the fpu CSR register for this operation.
	 * If an exception is required, generate a tidy SIGFPE exception,
	 * without updating the result register.
	 * Note: cause exception bits do not accumulate, they are rewritten
	 * for each op; only the flag/sticky bits accumulate.
	 */
	ctx->sr = (ctx->sr & ~FPU_CSR_ALL_X) | rcsr;
	if ((ctx->sr >> 5) & ctx->sr & FPU_CSR_ALL_E) {
		/*printk ("SIGFPE: fpu csr = %08x\n",ctx->sr); */
		return SIGFPE;
	}

	/* 
	 * Now we can safely write the result back to the register file.
	 */
	switch (rfmt) {
	case -1: {
#if __mips >= 4
		cond = fpucondbit[MIPSInst_FD(ir) >> 2];
#else
		cond = FPU_CSR_COND;
#endif
		if (rv.w)
			ctx->sr |= cond;
		else
			ctx->sr &= ~cond;
		break;
	}
#if !defined(SINGLE_ONLY_FPU)
	case d_fmt:
		DPTOREG(rv.d, MIPSInst_FD(ir));
		break;
#endif
	case s_fmt:
		SPTOREG(rv.s, MIPSInst_FD(ir));
		break;
	case w_fmt:
		SITOREG(rv.w, MIPSInst_FD(ir));
		break;
#if __mips64 && !defined(SINGLE_ONLY_FPU)
	case l_fmt:
		DITOREG(rv.l, MIPSInst_FD(ir));
		break;
#endif
	default:
		return SIGILL;
	}

	return 0;
}


/*
 * Emulate the floating point instruction at EPC, and continue to run until we
 * hit a non-fp instruction, or a backward branch.  This cuts down dramatically
 * on the per instruction exception overhead.
 */
int fpu_emulator_cop1Handler(struct pt_regs *xcp)
{
	struct mips_fpu_soft_struct *ctx = &current->thread.fpu.soft;
	unsigned long oldepc, prevepc;
	mips_instruction insn, *insnp;
	int sig = 0;

	oldepc = xcp->cp0_epc;
	do {
		prevepc = xcp->cp0_epc;

		/*
		 * This is a braindead way to do it but the only sane way I
		 * found to keep the 64-bit egcs 1.1.2 from crashing.
		 */
		insnp = (mips_instruction *) xcp->cp0_epc;
		if (verify_area(VERIFY_READ, insnp, 4) ||
		    __get_user(insn, insnp)) {
			fpuemuprivate.stats.errors++;
			return SIGBUS;
		}
		if (insn == 0)
			xcp->cp0_epc += 4;	/* skip nops */
		else {
			/* Update ieee754_csr. Only relevant if we have a
			   h/w FPU */
			ieee754_csr.nod = (ctx->sr & 0x1000000) != 0;
			ieee754_csr.rm = ieee_rm[ctx->sr & 0x3];
			ieee754_csr.cx = (ctx->sr >> 12) & 0x1f;
			sig = cop1Emulate(xcp, ctx);
		}

		if (mips_cpu.options & MIPS_CPU_FPU)
			break;
		if (sig)
			break;

		if (current->need_resched)
			schedule();
	} while (xcp->cp0_epc > prevepc);

	/* SIGILL indicates a non-fpu instruction */
	if (sig == SIGILL && xcp->cp0_epc != oldepc)
		/* but if epc has advanced, then ignore it */
		sig = 0;

	return sig;
}
