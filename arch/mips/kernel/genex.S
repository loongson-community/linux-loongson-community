/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 2000, 2001, 2003 Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2001 MIPS Technologies, Inc.
 */
#include <linux/config.h>
#include <linux/init.h>

#include <asm/asm.h>
#include <asm/cacheops.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>
#include <asm/fpregdef.h>

		__INIT

		.set	reorder

		NESTED(except_vec1_generic, 0, sp)
		PANIC("Exception vector 1 called")
		END(except_vec1_generic)

		/*
		 * General exception vector.  Used for all CPUs except R4000
		 * and R4400 SC and MC versions.
		 */
		NESTED(except_vec3_generic, 0, sp)
#if R5432_CP0_INTERRUPT_WAR
		mfc0	k0, CP0_INDEX
#endif
		mfc0	k1, CP0_CAUSE
		la	k0, exception_handlers
		andi	k1, k1, 0x7c
		addu	k0, k0, k1
		lw	k0, (k0)
		jr	k0
		END(except_vec3_generic)
		.set	at

		/* General exception vector R4000 version. */
		NESTED(except_vec3_r4000, 0, sp)
		.set	push
		.set    mips3
		.set	noat
		mfc0	k1, CP0_CAUSE
		li	k0, 31<<2
		andi	k1, k1, 0x7c
		.set	noreorder
		beq	k1, k0, handle_vced
		 li	k0, 14<<2
		beq	k1, k0, handle_vcei
		 lui	k0, %hi(exception_handlers)
		addiu	k0, %lo(exception_handlers)
		.set	reorder
		addu	k0, k0, k1
		lw	k0, (k0)
		jr	k0

		/*
		 * Big shit, we now may have two dirty primary cache lines for
		 * the same physical address.  We can savely invalidate the
		 * line pointed to by c0_badvaddr because after return from
		 * this exception handler the load / store will be re-executed.
		 */
handle_vced:
		mfc0	k0, CP0_BADVADDR
		li	k1, -4
		and	k0, k1
		mtc0	zero, CP0_TAGLO
		cache	Index_Store_Tag_D,(k0)
		cache	Hit_Writeback_Inv_SD,(k0)
#ifdef CONFIG_PROC_FS
		lui	k0, %hi(vced_count)
		lw	k1, %lo(vced_count)(k0)
		addiu	k1, 1
		sw	k1, %lo(vced_count)(k0)
#endif
		eret

handle_vcei:
		mfc0	k0, CP0_BADVADDR
		cache	Hit_Writeback_Inv_SD, (k0)	# also cleans pi
#ifdef CONFIG_PROC_FS
		lui	k0, %hi(vcei_count)
		lw	k1, %lo(vcei_count)(k0)
		addiu	k1, 1
		sw	k1, %lo(vcei_count)(k0)
#endif
		eret
		.set    pop
		END(except_vec3_r4000)

/*
 * Special interrupt vector for MIPS64 ISA & embedded MIPS processors.
 * This is a dedicated interrupt exception vector which reduces the
 * interrupt processing overhead.  The jump instruction will be replaced
 * at the initialization time.
 *
 * Be careful when changing this, it has to be at most 128 bytes
 * to fit into space reserved for the exception handler.
 */
NESTED(except_vec4, 0, sp)
1:	j	1b			/* Dummy, will be replaced */
	END(except_vec4)

	/*
	 * EJTAG debug exception handler.
	 * The EJTAG debug exception entry point is 0xbfc00480, which
	 * normally is in the boot PROM, so the boot PROM must do a
	 * unconditional jump to this vector.
	 */
	NESTED(except_vec_ejtag_debug, 0, sp)
	j	ejtag_debug_handler
	 nop
	END(except_vec_ejtag_debug)

	__FINIT

	/*
	 * EJTAG debug exception handler.
	 */
	NESTED(ejtag_debug_handler, PT_SIZE, sp)
	.set	noat
	.set	noreorder
	mtc0	k0, CP0_DESAVE
	mfc0	k0, CP0_DEBUG

	sll	k0, k0, 30	# Check for SDBBP.
	bgez	k0, ejtag_return

	la	k0, ejtag_debug_buffer
	sw	k1, 0(k0)
	SAVE_ALL
	jal	ejtag_exception_handler
	 move	a0, sp
	RESTORE_ALL
	la	k0, ejtag_debug_buffer
	lw	k1, 0(k0)

ejtag_return:
	mfc0	k0, CP0_DESAVE
	.set	mips32
	deret
	.set	mips0
	 nop
	.set	at
	END(ejtag_debug_handler)

	/*
	 * This buffer is reserved for the use of the EJTAG debug
	 * handler.
	 */
	.data
	EXPORT(ejtag_debug_buffer)
	.fill	4
	.previous

	__INIT

	/*
	 * NMI debug exception handler for MIPS reference boards.
	 * The NMI debug exception entry point is 0xbfc00000, which
	 * normally is in the boot PROM, so the boot PROM must do a
	 * unconditional jump to this vector.
	 */
	NESTED(except_vec_nmi, 0, sp)
	j	nmi_handler
	 nop
	END(except_vec_nmi)

	__FINIT

	NESTED(nmi_handler, PT_SIZE, sp)
	.set	noat
	.set	noreorder
	.set	mips3
	SAVE_ALL
	jal	nmi_exception_handler
 	move	a0, sp
	RESTORE_ALL
	eret
	.set	at
	.set	mips0
	END(nmi_handler)

/*
 * Build a default exception handler for the exceptions that don't need
 * special handlers.  If you didn't know yet - I *like* playing games with
 * the C preprocessor ...
 */
#define __BUILD_clear_none(exception)
#define __BUILD_clear_sti(exception)					\
		STI
#define __BUILD_clear_cli(exception)					\
		CLI
#define __BUILD_clear_kmode(exception)					\
		KMODE
#define __BUILD_clear_fpe(exception)                                    \
		cfc1	a1,fcr31;                                       \
		li	a2,~(0x3f<<12);                                 \
		and	a2,a1;                                          \
		ctc1	a2,fcr31;                                       \
		STI
#define __BUILD_clear_ade(exception)                                    \
		.set	reorder;					\
		MFC0	t0,CP0_BADVADDR;                                \
		.set	noreorder;					\
		REG_S	t0,PT_BVADDR(sp);                               \
		KMODE
#define __BUILD_silent(exception)

#define fmt "Got %s at %08lx.\n"

#define __BUILD_verbose(exception)                                      \
		la	a1,8f;                                          \
		TEXT	(#exception);                                   \
		REG_L	a2,PT_EPC(sp);                                  \
		PRINT(fmt)
#define __BUILD_count(exception)                                        \
		.set	reorder;                                        \
		lw	t0,exception_count_##exception;                 \
		.set	noreorder;                                      \
		addiu	t0, 1;                                          \
		sw	t0,exception_count_##exception;                 \
		.data;                                                  \
EXPORT(exception_count_##exception);                                    \
		.word	0;                                              \
		.previous;
#define BUILD_HANDLER(exception,handler,clear,verbose)                  \
		.align	5;                                              \
		NESTED(handle_##exception, PT_SIZE, sp);                \
		.set	noat;                                           \
		SAVE_ALL;                                               \
		FEXPORT(handle_##exception##_int);			\
		__BUILD_clear_##clear(exception);                       \
		.set	at;                                             \
		__BUILD_##verbose(exception);                           \
		jal	do_##handler;                                   \
		 move	a0, sp;                                         \
		j	ret_from_exception;                             \
		 nop;                                                   \
		END(handle_##exception)

		BUILD_HANDLER(adel,ade,ade,silent)		/* #4  */
		BUILD_HANDLER(ades,ade,ade,silent)		/* #5  */
		BUILD_HANDLER(ibe,be,cli,silent)		/* #6  */
		BUILD_HANDLER(dbe,be,cli,silent)		/* #7  */
		BUILD_HANDLER(bp,bp,kmode,silent)		/* #9  */
		BUILD_HANDLER(ri,ri,kmode,silent)		/* #10 */
		BUILD_HANDLER(cpu,cpu,kmode,silent)		/* #11 */
		BUILD_HANDLER(ov,ov,kmode,silent)		/* #12 */
		BUILD_HANDLER(tr,tr,kmode,silent)		/* #13 */
		BUILD_HANDLER(fpe,fpe,fpe,silent)		/* #15 */
		BUILD_HANDLER(mdmx,mdmx,sti,silent)		/* #22 */
		BUILD_HANDLER(watch,watch,sti,silent)		/* #23 */
		BUILD_HANDLER(mcheck,mcheck,cli,silent)		/* #24 */
		BUILD_HANDLER(reserved,reserved,kmode,silent)	/* others */
