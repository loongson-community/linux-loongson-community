/*
 * Suspend support specific for ST LS2F cpus.
 *
 * Copyright (c) 2009 Arnaud Patard <apatard@mandriva.com>
 */
#include <asm/asm.h>
#include <asm/asmmacro.h>
#include <asm/asm-offsets.h>

	.set noreorder
	.align 5

LEAF(make_sum)
        PTR_LA          t0, _text
        LONG_L          t3, (t0)
	daddiu 		t2,t3,0
        PTR_LA          t1, __bss_stop - LONGSIZE
1:
        PTR_ADDIU       t0, LONGSIZE
        LONG_L          t3, (t0)
	daddu		t2,t2,t3
        bne             t0, t1, 1b
	nop
	
	jr ra
	nop
END(make_sum)

LEAF(ls2f_save_regs)
	/* save callee saved registers */
#ifndef CONFIG_HIBERNATION
	LONG_S		s0, saved_context_s0
	LONG_S		s1, saved_context_s1
	LONG_S		s2, saved_context_s2
	LONG_S		s3, saved_context_s3
	LONG_S		s4, saved_context_s4
	LONG_S		s5, saved_context_s5
	LONG_S		s6, saved_context_s6
	LONG_S		s7, saved_context_s7
	LONG_S		s8, saved_context_s8
	LONG_S		gp, saved_context_gp
	LONG_S		sp, saved_context_sp
	LONG_S		ra, saved_context_ra
#endif
	LONG_S		k0, saved_context_k0
	LONG_S		k1, saved_context_k1
	LONG_S		jp, saved_context_jp


	lui		t0,0xa000
	ori		t0,t0, 0x0100
	ld		t1, 56(sp)
	sd		t1, 8(t0)

	/* save cp0 regs */
	mfc0		k0, CP0_STATUS
	LONG_S		k0, saved_context_status
	dmfc0		k0, CP0_CONTEXT
	LONG_S		k0, saved_context_context
	mfc0		k0, CP0_INDEX
	LONG_S		k0, saved_context_index
	dmfc0		k0, CP0_ENTRYLO0
	LONG_S		k0, saved_context_entrylo0
	dmfc0		k0, CP0_ENTRYLO1
	LONG_S		k0, saved_context_entrylo1
	mfc0		k0, CP0_PAGEMASK
	LONG_S		k0, saved_context_pagemask
	mfc0		k0, CP0_WIRED
	LONG_S		k0, saved_context_wired
	dmfc0		k0, CP0_ENTRYHI
	LONG_S		k0, saved_context_entryhi
	mfc0		k0, CP0_DIAGNOSTIC
	LONG_S		k0, saved_context_diag
	mfc0		k0, CP0_TAGLO
	LONG_S		k0, saved_context_taglo
	mfc0		k0, CP0_TAGHI
	LONG_S		k0, saved_context_taghi

	/* flush caches */
	PTR_L		t0, __flush_cache_all
	jalr		t0
	nop

	jal		make_sum
	nop
	lui		t0,0xa000
	ori		t0,t0, 0x0120
	sd		t2,(t0)

	/* jump to standby routine */
	PTR_LA		k0, ls2f_board_standby
	jr		k0
	nop

	/* restore regs  - should never get there */
#ifndef CONFIG_HIBERNATION
	LONG_L		s0, saved_context_s0
	LONG_L		s1, saved_context_s1
	LONG_L		s2, saved_context_s2
	LONG_L		s3, saved_context_s3
	LONG_L		s4, saved_context_s4
	LONG_L		s5, saved_context_s5
	LONG_L		s6, saved_context_s6
	LONG_L		s7, saved_context_s7
	LONG_L		s8, saved_context_s8
	LONG_L		gp, saved_context_gp
	LONG_L		sp, saved_context_sp
	LONG_L		ra, saved_context_ra
#endif
	LONG_L		jp, saved_context_jp
	LONG_L		k0, saved_context_k0
	LONG_L		k1, saved_context_k1

	jr		ra
	nop

END(ls2f_save_regs)

LEAF(ls2f_suspend_ret)
	jal		make_sum
	nop
	lui		t0,0xa000
	ori		t0,t0, 0x0120
	ld		t3,(t0)
	beq		t2,t3,1000f
	nop
	lui		t0,0xa000
	ori		t0,t0,0x0100
	lw		t1,0x6f6c7374
	sw		t1, (t0)
	sw		t1, 4(t0)
	PTR_LA		k0, machine_restart
	jr		k0
	nop

1000:
	lui		t0,0xa000
	ori		t0,t0, 0x0110
	lui		t1,0xaaaa
	sd		t1,0(t0)

	/* restore cp0 regs */
	LONG_L		k0, saved_context_taglo
	mtc0		k0, CP0_TAGLO
	LONG_L		k0, saved_context_taghi
	mtc0		k0, CP0_TAGHI
	LONG_L		k0, saved_context_diag
	dmtc0		k0, CP0_DIAGNOSTIC
	LONG_L		k0, saved_context_entryhi
	dmtc0		k0, CP0_ENTRYHI
	LONG_L		k0, saved_context_wired
	mtc0		k0, CP0_WIRED
	LONG_L		k0, saved_context_pagemask
	mtc0		k0, CP0_PAGEMASK
	LONG_L		k0, saved_context_entrylo1
	dmtc0		k0, CP0_ENTRYLO1
	LONG_L		k0, saved_context_entrylo0
	dmtc0		k0, CP0_ENTRYLO0
	LONG_L		k0, saved_context_index
	mtc0		k0, CP0_INDEX
	LONG_L		k0, saved_context_context
	dmtc0		k0, CP0_CONTEXT
	LONG_L		k0, saved_context_status
	mtc0		k0, CP0_STATUS

	/* flush tlb */
	jal		local_flush_tlb_all
	nop

	/* restore registers */
#ifndef CONFIG_HIBERNATION
	LONG_L		s0, saved_context_s0
	LONG_L		s1, saved_context_s1
	LONG_L		s2, saved_context_s2
	LONG_L		s3, saved_context_s3
	LONG_L		s4, saved_context_s4
	LONG_L		s5, saved_context_s5
	LONG_L		s6, saved_context_s6
	LONG_L		s7, saved_context_s7
	LONG_L		s8, saved_context_s8
	LONG_L		gp, saved_context_gp
	LONG_L		sp, saved_context_sp
	LONG_L		ra, saved_context_ra
#endif
	LONG_L		jp, saved_context_jp
	LONG_L		k0, saved_context_k0
	LONG_L		k1, saved_context_k1

	lui		t0,0xa000
	ori		t0,t0, 0x0100
	ld		t1, 56(sp)
	sd		sp, (t0)
	sd		t1,0x10(t0)

	jr		ra
	nop
END(ls2f_suspend_ret)
