/*
 * include/asm-mips/system.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 by Ralf Baechle
 */

#ifndef _ASM_MIPS_SYSTEM_H_
#define _ASM_MIPS_SYSTEM_H_

#include <linux/types.h>
#include <asm/segment.h>
#include <asm/mipsregs.h>

/*
 * move_to_user_mode() doesn't switch to user mode on the mips, since
 * that would run us into problems: The kernel is located at virtual
 * address 0x80000000. If we now would switch over to user mode, we
 * we would immediately get an address error exception.
 * Anyway - we don't have problems with a task running in kernel mode,
 * as long it's code is foolproof.
 */
#define move_to_user_mode()

#define sti() \
__asm__ __volatile__(                    \
	".set\tnoat\n\t"                 \
	"mfc0\t$1,"STR(CP0_STATUS)"\n\t" \
	"ori\t$1,$1,0x1f\n\t"            \
	"xori\t$1,$1,0x1e\n\t"           \
	"mtc0\t$1,"STR(CP0_STATUS)"\n\t" \
	".set\tat"                       \
	: /* no outputs */               \
	: /* no inputs */                \
	: "$1")

#define cli() \
__asm__ __volatile__(                    \
	".set\tnoat\n\t"                 \
	"mfc0\t$1,"STR(CP0_STATUS)"\n\t" \
	"ori\t$1,$1,1\n\t"               \
	"xori\t$1,$1,1\n\t"              \
	"mtc0\t$1,"STR(CP0_STATUS)"\n\t" \
	".set\tat"                       \
	: /* no outputs */               \
	: /* no inputs */                \
	: "$1")

#define nop() __asm__ __volatile__ ("nop")

extern ulong IRQ_vectors[256];
extern ulong exception_handlers[256];

#define set_intr_gate(n,addr) \
	IRQ_vectors[n] = (ulong) (addr)

#define set_except_vector(n,addr) \
	exception_handlers[n] = (ulong) (addr)

/*
 * atomic exchange of one word
 *
 * Fixme: This works only on MIPS ISA >=3
 */
#define atomic_exchange(m,r) \
	__asm__ __volatile__( \
		"1:\tll\t$8,(%2)\n\t" \
		"move\t$9,%0\n\t" \
		"sc\t$9,(%2)\n\t" \
		"beq\t$0,$9,1b\n\t" \
		: "=r" (r) \
		: "0" (r), "r" (&(m)) \
		: "$8","$9","memory");

#define save_flags(x)                    \
__asm__ __volatile__(                    \
	"mfc0\t%0,$12\n\t"               \
	: "=r" (x))                      \

#define restore_flags(x)                 \
__asm__ __volatile__(                    \
	"mtc0\t%0,$12\n\t"               \
	: /* no output */                \
	: "r" (x));                      \

#endif /* _ASM_MIPS_SYSTEM_H_ */
