/*
 * r4k_switch.S: R4xx0 specific task switching code.
 *
 * Copyright (C) 1994, 1995, 1996 by Ralf Baechle and Andreas Busse
 *
 * Multi-cpu abstraction and macros for easier reading:
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#include <asm/asm.h>
#include <asm/bootinfo.h>
#include <asm/cachectl.h>
#include <asm/current.h>
#include <asm/fpregdef.h>
#include <asm/mipsconfig.h>
#include <asm/mipsregs.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>

#include <asm/asmmacro.h>

	.set	noreorder
	.set	mips3
	.align	5
	LEAF(r4xx0_resume)
	mfc0	t1, CP0_STATUS
	sw	t1, THREAD_STATUS($28)
	ori	t2, t1, 0x1f
	xori	t2, t2, 0x1e
	mtc0	t2, CP0_STATUS
	CPU_SAVE_NONSCRATCH($28)
	sll	t2, t1, 2		# Save floating point state
	bgez	t2, 2f
	 sw	ra, THREAD_REG31($28)
	sll	t2, t1, 5
	bgez	t2, 1f
	 swc1	$f0, (THREAD_FPU + 0x00)($28)
	FPU_SAVE_16ODD($28)
1:
	FPU_SAVE_16EVEN($28, t1)		# clobbers t1
2:
	move	$28, a0
	lw	a3, TASK_MM($28)
	lw	a2, THREAD_STATUS($28)
	lw	a3, MM_CONTEXT(a3)
	ori	t1, a2, 1		# restore fpu, pipeline magic
	andi	a3, a3, 0xff
	xori	t1, t1, 1
	mtc0	a3, CP0_ENTRYHI
	mtc0	t1, CP0_STATUS
	sll	t0, a2, 2
	bgez	t0, 2f
	 sll	t0, a2, 5
	bgez	t0, 1f
	 lwc1	$f0, (THREAD_FPU + 0x00)($28)
	FPU_RESTORE_16ODD($28)
1:
	FPU_RESTORE_16EVEN($28, t0)	# clobbers t0
2:
	CPU_RESTORE_NONSCRATCH($28)
	lw	t0, THREAD_KSP($28)
	sw	t0, kernelsp
	jr	ra
	 mtc0	a2, CP0_STATUS
	END(r4xx0_resume)
