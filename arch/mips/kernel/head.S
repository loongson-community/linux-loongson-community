/*
 *  arch/mips/kernel/head.S
 *
 *  Copyright (C) 1994, 1995 Waldorf Electronics, 1996 Paul M. Antoine
 *  Written by Ralf Baechle and Andreas Busse
 *  Modified for DECStation and hence R3000 support by Paul M. Antoine
 *  Additional R3000 support by Didier Frick <dfrick@dial.eunet.ch>
 *  for ACN S.A, Copyright (C) 1996 by ACN S.A
 *
 *  Head.S contains the MIPS exception handler and startup code.
 *
 *  FIXME: Note that the #ifdef's for R4X00 assume R3000 is the #else
 *	   case, which is a little naughty.  We also do NOT need the
 *	   dec_entry goo at the begining of all this - PMA
 *  FIXME: This #ifdef stuff is ugly and I should move the tlb/exception
 *         handler code out into some other file - Ralf
 *  Take the zillions of (_MIPS_ISA == _MIPS_ISA_MIPSx) as a temporary
 *  solution.  I know how they look ...
 */
#include <linux/config.h>		/* For the DECstation hacks */
#include <linux/tasks.h>

#include <asm/addrspace.h>
#include <asm/asm.h>
#include <asm/processor.h>
#include <asm/regdef.h>
#include <asm/cache.h>
#include <asm/mipsregs.h>
#include <asm/mipsconfig.h>
#include <asm/stackframe.h>
#include <asm/bootinfo.h>

#define PAGE_SIZE	0x1000

/*
 * FIXME: I still think the following should be in an include file (see
 *	  also the reference in arch/mips/mips1/r3000.S - PMA
 */
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
#define MODE_GLOBAL	0x0100	/* shared for all processes */
#define MODE_ALIAS	0x00e0
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS5)
#define MODE_GLOBAL	0x0001	/* shared for all processes */
#define MODE_ALIAS	0x0016	/* uncachable */
#endif

/*
 * The two symbols begin_except and end_except mark the range that is copied
 * to KSEG0 on startup.
 */
EXPORT(begin_except)
		.text
/*	
 * This is space for the interrupt handlers.
 * After trap_init() they are located at virtual address KSEG0.
 *
 * For some machine where the kernel doesn't get directly loaded to KSEG0
 * the exceptionhandler get copied to KSEG0.  They therefore must be
 * relocatable code.
 */
		/*
		 * TLB refill, EXL == 0
		 */
		.set	noreorder
		.set	noat
		LEAF(except_vec0)
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
		mfc0	k1,CP0_CONTEXT
		nop
		lw	k0,(k1)			# May cause another exception
		mfc0	k1,CP0_EPC		# Get the return address
		srl	k0,12			# Convert to EntryLo format
		mtc0	k0,CP0_ENTRYLO0
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS5)
		.set	mips3
		dmfc0	k1,CP0_CONTEXT
		dsra	k1,1
		lwu	k0,(k1)			# May cause another exception
		lwu	k1,4(k1)
		dsrl	k0,6			# Convert to EntryLo format
		dsrl	k1,6			# Convert to EntryLo format
		dmtc0	k0,CP0_ENTRYLO0
		dmtc0	k1,CP0_ENTRYLO1
#endif
#ifndef CONFIG_OPTIMIZE_R4600
		nop				# Needed for R4[04]00 pipeline
#endif
		tlbwr
		nop				# Needed for R4[04]00 pipeline
		nop
#ifndef CONFIG_OPTIMIZE_R4600
		nop
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
		jr	k1
		rfe
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS5)
		eret
		/*
		 * Partial workaround for R4000 bug.  For explanation see
		 * MIPS docs.  Note that this that obscure that it wont
		 * almost never happen.  Well, but Mips writes about it's bugs.
		 */
		nop
		eret
#endif
		END(except_vec0)

/******************************************************************************/

		/*
		 * XTLB refill, EXL == 0
		 * Should never be reached on R4000.
		 */
		.org	except_vec0+0x80
		NESTED(except_vec1, 0, sp)
		.set	noat
		/*
		 * Register saving is delayed as long as we don't know
		 * which registers really need to be saved.
		 */
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
		mfc0	k1,CP0_CONTEXT
		nop
		lw	k0,(k1)			# May cause another exception
		mfc0	k1,CP0_EPC		# Get the return address
		srl	k0,12			# Convert to EntryLo format
		mtc0	k0,CP0_ENTRYLO0
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS5)
		mfc0	k1,CP0_CONTEXT
		dsra	k1,1
		lwu	k0,(k1)			# May cause another exception
		lwu	k1,4(k1)
		dsrl	k0,6			# Convert to EntryLo format
		dsrl	k1,6			# Convert to EntryLo format
		dmtc0	k0,CP0_ENTRYLO0
		dmtc0	k1,CP0_ENTRYLO1
#endif
		nop				# Needed for R4[04]00 pipeline
		tlbwr
		nop				# Needed for R4[04]00 pipeline
		nop
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
		jr	k1
		rfe
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS5)
		nop
		eret
#endif
		/*
		 * Partial workaround for R4000 bug.  For explanation see
		 * MIPS docs.  Note that this that obscure that it wont
		 * almost never happen.  Well, but Mips writes about it's bugs.
		 */
		nop
		eret
		END(except_vec1)

/******************************************************************************/

		/*
		 * Cache Error
		 */
		.org	except_vec1+0x80
		LEAF(except_vec2)
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
		/*
		 * On the R3000, this is the "Uncached TLB Miss" handler.
		 */
		j	except_vec0
		nop
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS5)
		/*
		 * Famous last words: unreached
		 */
		mfc0	a1,CP0_ERROREPC
		PRINT("Cache error exception: c0_errorepc == %08x\n")
1:		j	1b
		nop
#endif
		END(except_vec2)

/******************************************************************************/

		/*
		 * General exception vector.
		 */
		.org	except_vec2+0x80
		NESTED(except_vec3, 0, sp)
		.set	noat
		/*
		 * Register saving is delayed as long as we don't know
		 * which registers really need to be saved.
		 * Except for k1 which MUST be preserved to allow
		 * nested TLB refill exceptions on the R3000.
		 */
		mfc0	k1,CP0_CAUSE
		la	k0,exception_handlers
		/*
		 * Next lines assumes that the used CPU type has max.
		 * 32 different types of exceptions. We might use this
		 * to implement software exceptions in the future.
		 */
		andi	k1,0x7c
		addu	k0,k1
		lw	k0,(k0)
		NOP
		jr	k0
		nop
		END(except_vec3)
		.set	at

EXPORT(end_except)

/******************************************************************************/

/*
 * Kernel entry
 */
		.set	noreorder

		NESTED(kernel_entry, 16, sp)
		/*
		 * The following two symbols are used for kernel profiling.
		 */
		EXPORT(stext)
		EXPORT(_stext)

		/*
		 * Initialize the global pointer, if required.
		 */
		LOAD_GP

		/*
		 * First setup stack for kernel and init
		 */
		la	sp,init_user_stack+(KERNEL_STACK_SIZE-4*SZREG)
		la	t0,init_kernel_stack+(KERNEL_STACK_SIZE)
		LONG_S	t0,kernelsp

		/*
		 * Clear BSS first so that there are no surprises...
		 */
		la	t0,_edata
		la	t1,_end
		sb	zero,(t0)
1:		addiu	t0,1
#if (_MIPS_ISA == _MIPS_ISA_MIPS1)
		/*
		 * Paul, this clears one word too much - Ralf
		 */
		bne	t0,t1,1b
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS2) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS3) || \
    (_MIPS_ISA == _MIPS_ISA_MIPS4)
		bnel	t0,t1,1b
#endif
		sb	zero,(t0)			# delay slot

 		/*
 		 * Get the memory upper limit the bootloader passed to us
                 * in a0
 		 */
		sw	a0,mips_memory_upper

		/*
		 * Get the very one tags we need early in the boot process
  		 */
		jal	bi_EarlySnarf
		nop

		/*
		 * Initialize low level part of memory management
		 * First flush the TLB to make sure that we don't get a
		 * TLB shutdown during wire_mappings.
		 */
		jal	tlbflush
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
		nop
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
		mtc0	zero,CP0_WIRED			# delay slot
#endif
		jal	wire_mappings
		nop

		/*
		 * Disable coprocessors
		 */
		mfc0	t0,CP0_STATUS
		li	t1,~(ST0_CU1|ST0_CU2|ST0_CU3)
		and	t0,t1
		li	t1,ST0_CU0
		or	t0,ST0_CU0
		mtc0	t0,CP0_STATUS

1:		jal	start_kernel
		nop					# delay slot
		/*
		 * Main should never return here, but
		 * just in case, we know what happens.
		 */
		b	1b
		nop					# delay slot
		END(kernel_entry)

/*
 * wire_mappings - used to map hardware registers
 */
		LEAF(wire_mappings)
		/*
 		 * Get base address of map0 table for the
		 * the board we're running on
		 */
		lw 	t1,mips_machgroup	# mips_machgroup is set by 
	                                        # bi_EarlySnarf() 
 		la	t0,map0table
		sll	t1,PTRLOG		# machgroup used as index
		addu	t0,t1
		lw	t1,mips_machtype	# mips_machtype is set by 
						# bi_EarlySnarf() 
		lw	t0,(t0)			# load table @ for the group
 		sll	t1,PTRLOG		# machtype used as index
 		addu	t0,t1
		lw	t0,(t0)			# load table @ for the box
		nop
		/*
		 * Get number of wired TLB entries and
		 * loop over selected map0 table.
		 */
		lw	t1,(t0)			# number of wired TLB entries
		move	t2,zero			# TLB entry counter
		addiu	t3,t1,1			# wire one additional entry
		beqz	t1,2f			# null, exit
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
		nop
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
		mtc0	t3,CP0_WIRED		# delay slot
#endif
		addiu	t0,8
1:		lw	t4,24(t0)		# PageMask
		ld	t5,0(t0)		# entryHi
		ld	t6,8(t0)		# entryLo0
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
		ld	t7,16(t0)		# entryLo1
#endif
		addiu	t2,1			# increment ctr
		mtc0	t2,CP0_INDEX		# set TLB entry
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
		nop
		mtc0	t5,CP0_ENTRYHI
		nop
		mtc0	t6,CP0_ENTRYLO0
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
		mtc0	t4,CP0_PAGEMASK
		dmtc0	t5,CP0_ENTRYHI
		dmtc0	t6,CP0_ENTRYLO0
		dmtc0	t7,CP0_ENTRYLO1
#endif
		addiu	t0,32
		bne	t1,t2,1b		# next TLB entry
 		tlbwi 				# delay slot

		/*
		 * We use only 4k pages. Therefore the PageMask register
		 * is expected to be setup for 4k pages.
		 */
2:
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
		li	t0,PM_4K
		mtc0	t0,CP0_PAGEMASK

#endif
		/*
		 * Now map the pagetables
		 */
		mtc0	zero,CP0_INDEX
		la	t0,TLB_ROOT
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
		mtc0	t0,CP0_ENTRYHI
		nop
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
		dmtc0	t0,CP0_ENTRYHI
#endif
		la	t0,swapper_pg_dir-KSEG1
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
		srl	t0,12
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
		srl	t0,6
#endif
		ori	t0,(MODE_ALIAS|MODE_GLOBAL) # uncachable, dirty, valid
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
		mtc0	t0,CP0_ENTRYLO0
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
		dmtc0	t0,CP0_ENTRYLO0
		li	t0,MODE_GLOBAL
		dmtc0	t0,CP0_ENTRYLO1
#endif
		nop
		tlbwi				# delayed

		/*
		 * Load the context register with a value that allows
		 * it to be used as fast as possible in tlb exceptions.
		 * It is expected that this register's content will
		 * NEVER be changed.
		 */
		li	t0,TLBMAP
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
		srl	t0,1			# this is a guess!
		mtc0	t0,CP0_CONTEXT
#endif
#if (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4)
		dsll	t0,1
		dmtc0	t0,CP0_CONTEXT
#endif
		jr	ra			# delay slot
		nop
		END(wire_mappings)

		.data
/*
 * Build an entry for table of wired entries
 */
#define MAPDATA(q1,q2,q3,w1)                                    \
		.quad	q1;                                     \
		.quad	q2;                                     \
		.quad	q3;                                     \
		.word	w1;                                     \
		.word	0

/*
 * Initial mapping tables for supported Mips boards.
 * First item is always the number of wired TLB entries,
 * following by EntryHi/EntryLo pairs and page mask.
 * Since everything must be quad-aligned (8) we insert
 * some dummy zeros.
 *
 * Keep in mind that the PFN does not depend on the page size in the
 * TLB page mask register.  See milo's lib/dumptlb.c for how to decode
 * and encode these entries.  Don't see the same routine in the linux
 * kernel distribution, since it is older and unreliable.
 */

/*
 * Address table of mapping tables for supported Mips boards.
 * Add your own stuff here but don't forget to define your
 * target system in bootinfo.h
 */
/* First indirection level on the 'group' */
map0table:	PTR	map0table_unknown	# machgroup = unknown
		PTR	map0table_jazz		# machgroup = JAZZ
		PTR	map0table_dec		# machgroup = DEC
		PTR	map0table_arc		# machgroup = ARC
		PTR	map0table_sni_rm	# machgroup = SNI_RM
		PTR	map0table_acn		# machgroup = ACN
		.word	0			# pad

/* table for group 'unknown' */
map0table_unknown:	PTR	map0_dummy		# machtype = unknown
			.word	0			# pad

/* table for group 'Jazz' */
map0table_jazz:	PTR	map0_pica61		# Acer Pica-61
		PTR	map0_magnum4000		# MIPS Magnum 4000PC (RC4030)
		PTR	map0_magnum4000		# Olivetti M700 (*same* table)
		.word	0			# pad

/* table for group 'Dec' */
map0table_dec:	PTR	map0_dummy		# DEC Personal DECStation 5000/2x (for now)
		.word 	0			# pad

/* table for group 'ARC' */
map0table_arc: 	PTR	map0_rpc		# Deskstation rPC44
		PTR	map0_tyne		# Deskstation Tyne

/* table for group 'SNI_RM' */
map0table_sni_rm:	PTR	map0_sni_rm200_pci	# SNI RM200 PCI
			.word	0

/* table for group 'ACN' */
map0table_acn:	PTR	map0_dummy		# ACN mips board
		.word   0

/* dummy table */
map0_dummy:	.word	0			# 0 entries

		.align  3
/*
 * Deskstation rpc44 mappings.  This machine has its EISA bus at physical
 * address 0xa0000000 which we map for 32M, but that doesn't match EISA
 * spec.  Not sure what to do about this.  Its I/O ports are memory mapped
 * at physical memory location 0xb0000000.
 */
map0_rpc: 	.word   2			# no. of wired TLB entries
		.word	0			# pad for alignment

MAPDATA(0xffffffffe0000000, 0x02800017, 0x00000011, PM_16M)	# ISA Memory space
MAPDATA(0xffffffffe2000000, 0x02c00017, 0x00000011, PM_64K)	# ISA I/O Space

/*
 * Initial mappings for Deskstation Tyne boards.
 */
map0_tyne: 	.word   2			# no. of wired TLB entries
		.word	0			# pad for alignment

MAPDATA(0xffffffffe0000000, 0x04020017, 0x00000011, PM_1M)	# VESA DMA cache
MAPDATA(0xffffffffe2000000, 0x24000017, 0x04000017, PM_16M)	# VESA I/O and memory space

/*
 * Initial mapping for ACER PICA-61 boards.
 * FIXME: Remove or merge some of the mappings.
 */
map0_pica61:	.word  	7			# no. wired TLB entries
		.word	0			# dummy

MAPDATA(0xffffffffe0000000, 0x02000017, 0x00000011, PM_64K)	# Local I/O space
MAPDATA(0xffffffffe0100000, 0x03c00017, 0x00000011, PM_4K)	# Interrupt source register
MAPDATA(0xffffffffe0200000, 0x01800017, 0x01804017, PM_1M)	# Local video control
MAPDATA(0xffffffffe0400000, 0x01808017, 0x0180c017, PM_1M)	# Extended video control
MAPDATA(0xffffffffe0800000, 0x01000017, 0x01010017, PM_4M)	# Local video memory (BIOS mapping)
MAPDATA(0xffffffffe2000000, 0x02400017, 0x02440017, PM_16M)	# ISA I/O and ISA memory space (both 16M)
MAPDATA(0xffffffffffffe000, 0x00000001, 0x0001ffd7, PM_4K)	# PCR (???)

/*
 * Initial mapping for Mips Magnum 4000PC systems.
 * FIXME: Remove or merge some of the mappings.
 */
map0_magnum4000:
		.word  	8			# no. wired TLB entries
		.word	0			# dummy

MAPDATA(0xffffffffe1000000, 0x03ffc013, 0x00000011, PM_256K)	# 0
MAPDATA(0xffffffffe0000000, 0x02000017, 0x00000011, PM_64K)	# 1 local I/O
MAPDATA(0xffffffffe0100000, 0x03c00017, 0x00000011, PM_4K)	# 2 IRQ source
MAPDATA(0xffffffffe0200000, 0x01800017, 0x01804017, PM_1M)	# 3 local video ctrl
MAPDATA(0xffffffffe0400000, 0x01808017, 0x0180c017, PM_1M)	# 4 ext. video ctrl
MAPDATA(0xffffffffe0800000, 0x01000017, 0x01010017, PM_4M)	# 5 local video mem.
MAPDATA(0xffffffffe2000000, 0x02400017, 0x02440017, PM_16M)	# 6 ISA I/O and mem.
MAPDATA(0xffffffffffffe000, 0x00000001, 0x0001ffd7, PM_4K)	# 7 PCR

/*
 * The RM200 doesn't need any wired entries.
 */
map0_sni_rm200_pci:
		.word  	0			# no. wired TLB entries
		.word	0			# dummy

			.text

			.org	0x1000
			.globl	swapper_pg_dir
swapper_pg_dir		=	. + (KSEG1-KSEG0)

			.org	0x2000
			EXPORT(empty_bad_page)

			.org	0x3000
			EXPORT(empty_bad_page_table)

			.org	0x4000
			EXPORT(empty_zero_page)

			.org	0x5000
			EXPORT(invalid_pte_table)

			.org	0x6000

			EXPORT(cache_error_buffer)
			.fill	32*4,1,0

			.data
			EXPORT(kernelsp)
			PTR	0
