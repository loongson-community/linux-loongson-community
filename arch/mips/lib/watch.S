/*
 * Kernel debug stuff to use the Watch registers.
 * Usefull to find stack overflows etc.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995 Ralf Baechle
 */
#include <asm/asm.h>
#include <asm/mipsregs.h>

		.set	noreorder
/*
 * Parameter: a0 - logic address to watch
 *                 Currently only KSEG0 addresses are allowed!
 *            a1 - set bit #1 to trap on load references
 *                     bit #0 to trap on store references
 * Results  : none
 */
		LEAF(watch_set)
		li	t0,0x80000000
		subu	a0,t0
		ori	a0,7
		xori	a0,7
		or	a0,a1
		mtc0	a0,CP0_WATCHLO
		jr	ra
		mtc0	zero,CP0_WATCHHI
		END(watch_set)

/*
 * The stuff below are just some kernel debugging gadgets. It will
 * go away.
 */

/*
 * Parameter: none
 * Results  : none
 */
		LEAF(watch_clear)
		jr	ra
		mtc0	zero,CP0_WATCHLO
		END(watch_clear)

/*
 * Parameter: none
 * Results  : none
 */
		LEAF(get_sp)
		jr	ra
		move	v0,sp
		END(get_sp)

/*
 * Parameter: none
 * Results  : none
 */
		LEAF(get_ra)
		jr	ra
		move	v0,ra
		END(get_ra)

/*
 * Parameter: none
 * Results  : none
 */
		LEAF(get_status)
		jr	ra
		mfc0	v0,CP0_STATUS
		END(get_status)

/*
 * Parameter: none
 * Results  : none
 */
		NESTED(print_sp, 24, sp)
		.mask	0x80000000,16
		subu	sp,24
		sw	ra,16(sp)
		move	a1,sp
		PRINT("$sp == %08lx\n")
		lw	ra,16(sp)
		jr	ra
		addiu	sp,24
		END(print_sp)

/*
 * Parameter: none
 * Results  : none
 */
		NESTED(print_st, 24, sp)
		.mask	0x80000000,16
		subu	sp,24
		sw	ra,16(sp)
		mfc0	a1,CP0_STATUS
		PRINT("cp0_status == %08lx\n")
		lw	ra,16(sp)
		jr	ra
		addiu	sp,24
		END(print_st)
