/*
 *  mips/head.S
 *
 *  Copyright (C) 1994  Ralf Baechle
 *
 *  Head.S contains the MIPS 32-bit startup code.
 */

/*
 * prevent prototypes from being imported
 */
#define __ASSEMBLY__

#include <asm/segment.h>
#include <asm/cachectl.h>
#include <asm/mipsregs.h>
#include <asm/mipsconfig.h>
#include <asm/stackframe.h>
#include <asm/regdef.h>
#include <linux/tasks.h>


		.globl	_empty_bad_page
		.globl	_empty_bad_page_table
		.globl	_invalid_pg_table
		.globl	_empty_zero_page
		.globl	_tmp_floppy_area
		.globl	_floppy_track_buffer
		.globl	_swapper_pg_dir

		.set	noreorder

		.text
/*
 * This is space for the interrupt handlers.
 * They are located at virtual address 0x80000000 (physical 0x0)
 */
		/*
		 * TLB refill, EXL == 0
		 */
except_vec0:
		.set	noreorder
		.set	noat
		/*
		 * This TLB-refill handler is supposed never to cause
		 * another tlb-refill exception. Unmapped pages will
		 * cause another type of exception.
		 */
		dmfc0	k0,CP0_CONTEXT
		dsrl	k0,k0,1
		lwu	k0,(k1)
		lwu	k1,4(k1)
		dmtc0	k0,CP0_ENTRYLO0
		dmtc0	k0,CP0_ENTRYLO1
		tlbwr
		eret

		/*
		 * XTLB refill, EXL == 0 (X == 64-bit TLB)
		 */
		.org	except_vec0+0x80
except_vec1:
		/*
		 * Not used yet...
		 */
		eret

		/*
		 * Cache Error
		 */
		.org	except_vec1+0x80
except_vec2:
		/*
		 * Not used yet...
		 */
		eret

		/*
		 * General exception vector.
		 */
		.org	except_vec2+0x80
except_vec3:
		SAVE_ALL
		mfc0	t0,CP0_STATUS
		ori	t0,t0,0x1f
		xori	t0,t0,0x1f
		mtc0	t0,CP0_STATUS
		.set	at
		la	k0,_exception_handlers
		mfc0	k1,CP0_CAUSE
		andi	k1,k1,0x7c
		addu	k0,k0,k1
		lw	k0,(k0)
		FILL_LDS
		jr	k0
		nop


/******************************************************************************/

		/*
		 * The following data is expected to be at certain absolute
		 * addresses, which are hardwired in
		 * include/asm-mips/mipsconfig.h
		 * If the following offset is to short, the assembler will
		 * break with an assertion failure. You then will have to
		 * increase it and to fix the address in
		 * include/asm-mips/mipsconfig.h
		 */

		.org	except_vec3+0x100
		.globl	_kernelsp
_kernelsp:	.word	0

kernel_entry:

/*
 * Flush the TLB
 */
		dmtc0	zero,CP0_ENTRYHI
		dmtc0	zero,CP0_ENTRYLO0
		dmtc0	zero,CP0_ENTRYLO1
		li	t0,NUMBER_OF_TLB_ENTRIES-1
1:		mtc0	t0,CP0_INDEX
		tlbwi
		bne	zero,t0,1b
		subu	t0,t0,1

/*
 * Initialize memory management.
 * Wire mapping for port i/o space 0xe0000000 -> 0x9000000900000000
 */
		li	t0,3
		mtc0	t0,CP0_WIRED
		li	t0,PM_64K
		mtc0	t0,CP0_PAGEMASK
		la	t3,map0
		ld	t1,0(t3)
		ld	t2,8(t3)
		mtc0	zero,CP0_INDEX
		dmtc0	t1,CP0_ENTRYHI
		dmtc0	t2,CP0_ENTRYLO0
		dmtc0	zero,CP0_ENTRYLO1		/* Invalid page */
		tlbwi
		li	t0,PM_1M
		mtc0	t0,CP0_PAGEMASK
		ld	t1,16(t3)
		ld	t2,24(t3)
		li	t0,1
		mtc0	t0,CP0_INDEX
		dmtc0	t1,CP0_ENTRYHI
		dmtc0	t2,CP0_ENTRYLO0
		tlbwi
		ld	t1,32(t3)
		ld	t2,40(t3)
		li	t0,2
		mtc0	t0,CP0_INDEX
		dmtc0	t1,CP0_ENTRYHI
		dmtc0	t2,CP0_ENTRYLO0
		tlbwi

/*
 * We always use 4k pages. Therefore the PageMask register
 * is expected to be setup for 4k pages.
 */
		li	t0,PM_4K
		mtc0	t0,CP0_PAGEMASK

/*
 * Clear BSS first so that there are no surprises...
 */
		la	t0,__edata
		la	t1,__end
		sw	zero,(t0)
1:		addiu	t0,t0,4
		bnel	t0,t1,1b
		sw	zero,(t0)

/*
 * Copy bootup parameters out of the way. First 2kB of
 * _empty_zero_page is for boot parameters, second 2kB
 * is for the command line.
 */
#if 0
		movl $0x90000,%esi
		movl $_empty_zero_page,%edi
		movl $512,%ecx
		cld
		rep
		movsl
		xorl %eax,%eax
		movl $512,%ecx
		rep
		stosl
		cmpw $(CL_MAGIC),CL_MAGIC_ADDR
		jne 1f
		movl $_empty_zero_page+2048,%edi
		movzwl CL_OFFSET,%esi
		addl $(CL_BASE_ADDR),%esi
		movl $2048,%ecx
		rep
		movsb
#endif

		/*
		 * Preliminary stack...
		 */
		la	sp,0x80700000
		sw	sp,_kernelsp
6:
		jal	_start_kernel
		nop
		j	6b		# main should never return here, but
					# just in case, we know what happens.

#if 0
/* This is the default interrupt "handler" :-) */
int_msg:
		.asciz "Unknown interrupt\n"
.align 2
ignore_int:
		cld
		pushl %eax
		pushl %ecx
		pushl %edx
		push %ds
		push %es
		push %fs
		movl $(KERNEL_DS),%eax
		mov %ax,%ds
		mov %ax,%es
		mov %ax,%fs
		pushl $int_msg
		call _printk
		popl %eax
		pop %fs
		pop %es
		pop %ds
		popl %edx
		popl %ecx
		popl %eax
		iret
#endif

#define CACHELINES	512             /* number of cachelines    */

/*
 * Flush instruction/data caches
 *
 * Parameters: a0 - starting address to flush
 *             a1 - size of area to be flushed
 *             a2 - which caches to be flushed
 *
 * FIXME:      - ignores parameters
 *             - doesn't know about second level caches
 */

		.set	noreorder
		.globl	_cacheflush
		.text
_cacheflush:
		/*
		 * Flush the instruction cache
		 */
		lui	t0,0x8000
		li	t1,CACHELINES-1
1:		cache	0,0(t0)
		cache	0,32(t0)
		cache	0,64(t0)
		cache	0,96(t0)
		cache	0,128(t0)
		cache	0,160(t0)
		cache	0,192(t0)
		cache	0,224(t0)
		cache	0,256(t0)
		cache	0,288(t0)
		cache	0,320(t0)
		cache	0,352(t0)
		cache	0,384(t0)
		cache	0,416(t0)
		cache	0,448(t0)
		cache	0,480(t0)
		addiu	t0,t0,512
		bne	zero,t1,1b
		subu	t1,t1,1
		/*
		 * Flush the data cache
		 */
		lui	t0,0x8000
		li	t1,CACHELINES-1
1:		cache	1,0(t0)
		cache	1,32(t0)
		cache	1,64(t0)
		cache	1,96(t0)
		cache	1,128(t0)
		cache	1,160(t0)
		cache	1,192(t0)
		cache	1,224(t0)
		cache	1,256(t0)
		cache	1,288(t0)
		cache	1,320(t0)
		cache	1,352(t0)
		cache	1,384(t0)
		cache	1,416(t0)
		cache	1,448(t0)
		cache	1,480(t0)
		addiu	t0,t0,512
		bne	zero,t1,1b
		subu    t1,t1,1

		j	ra
		nop

		.globl	_beep
_beep:		lbu	t0,0xe0000061
		xori	t0,t0,3
		sb	t0,0xe0000061
		jr	ra
		nop

/*
 * Instead of Intel's strage and unportable segment descriptor magic
 * we difference user and kernel space by their address.
 * Kernel space (== physical memory) is mapped at 0x80000000,
 * User space is mapped at 0x0.
 */

			.data

			.globl	_segment_fs
			/*
			 * Inital wired mappings
			 */
map0:			.quad	0xc00000ffe0000000,0x24000017
			.quad	0xc00000ffe1000000,0x04000017
			.quad	0xc00000ffe2000000,0x04020017

/*
 * page 0 is made non-existent, so that kernel NULL pointer references get
 * caught. Thus the swapper page directory has been moved to 0x1000
 *
 * XXX Actually, the swapper page directory is at 0x1000 plus 1 megabyte,
 * with the introduction of the compressed boot code.  Theoretically,
 * the original design of overlaying the startup code with the swapper
 * page directory is still possible --- it would reduce the size of the kernel
 * by 2-3k.  This would be a good thing to do at some point.....
 */
			.text

			.org	0x1000
_swapper_pg_dir:
/*
 * The page tables are initialized to only 4MB here - the final page
 * tables are set up later depending on memory size.
 */
			.org	0x2000
_pg0:

			.org	0x3000
_empty_bad_page:

			.org	0x4000
_empty_bad_page_table:

			.org	0x5000
_invalid_pg_table:

			.org	0x6000
_empty_zero_page:

			.org	0x7000

/*
 * tmp_floppy_area is used by the floppy-driver when DMA cannot
 * reach to a buffer-block. It needs to be aligned, so that it isn't
 * on a 64kB border.
 */
_tmp_floppy_area:	.fill 1024,1,0
/*
 * floppy_track_buffer is used to buffer one track of floppy data: it
 * has to be separate from the tmp_floppy area, as otherwise a single-
 * sector read/write can mess it up. It can contain one full cylinder (sic) of
 * data (36*2*512 bytes).
 */
_floppy_track_buffer:	.fill 512*2*36,1,0

_segment_fs:		.word	KERNEL_DS
