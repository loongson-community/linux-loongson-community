/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1997, 1998, 2001 by Ralf Baechle
 */
#ifndef _ASM_BRANCH_H
#define _ASM_BRANCH_H

#include <asm/inst.h>
#include <asm/ptrace.h>
#include <asm/uaccess.h>
#include <asm/war.h>

static inline int delay_slot(struct pt_regs *regs)
{
#ifdef BDSLOT_WAR
	union mips_instruction insn;
	mm_segment_t seg;
#endif
	int ds;

	ds = regs->cp0_cause & CAUSEF_BD;
	if (ds)
		return ds;

#ifdef BDSLOT_WAR
	if (!user_mode(regs))
		set_fs(KERNEL_DS);
	__get_user(insn.word, (unsigned int *)regs->cp0_epc);
	set_fs(seg);

	switch (insn.i_format.opcode) {
	/*
	 * On some CPUs, if an unaligned access happens in a branch delay slot
	 * and the branch is not taken, EPC points at the branch instruction,
	 * but the BD bit in the cause register is not set.
	 */
	case bcond_op:
	case j_op:
	case jal_op:
	case beq_op:
	case bne_op:
	case blez_op:
	case bgtz_op:
	case beql_op:
	case bnel_op:
	case blezl_op:
	case bgtzl_op:
	case jalx_op:
		return 1;	
	}
#endif

	return 0;
}

static inline unsigned long exception_epc(struct pt_regs *regs)
{
	if (!delay_slot(regs))
		return regs->cp0_epc;

	return regs->cp0_epc + 4;
}

extern int __compute_return_epc(struct pt_regs *regs);

static inline int compute_return_epc(struct pt_regs *regs)
{
	if (!delay_slot(regs)) {
		regs->cp0_epc += 4;
		return 0;
	}

	return __compute_return_epc(regs);
}

#endif /* _ASM_BRANCH_H */
