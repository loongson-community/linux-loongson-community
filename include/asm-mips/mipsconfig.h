/*
 * include/asm-mips/mipsconfig.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 by Waldorf Electronics
 * written by Ralf Baechle
 */
#ifndef __ASM_MIPS_MIPSCONFIG_H
#define __ASM_MIPS_MIPSCONFIG_H

/*
 * This is the virtual address to which all ports are being mapped.
 * Must be a value that can be load with a lui instruction.
 */
#ifndef PORT_BASE
#define PORT_BASE		0xe2000000
#endif

/*
 * Pagetables are 4MB mapped at 0xe4000000
 * Must be a value that can be loaded with a single instruction.
 */
#define TLBMAP			0xe4000000

/*
 * The virtual address where we'll map the pagetables
 * For a base address of 0xe3000000 this is 0xe338c000
 * For a base address of 0xe4000000 this is 0xe4390000
 * FIXME: Gas computes the following expression with signed
 *        shift and therefore false
#define TLB_ROOT		(TLBMAP + (TLBMAP >> (12-2)))
 */
#define TLB_ROOT		0xe4390000

/*
 * Use this to activate extra TLB error checking
 */
#define CONFIG_DEBUG_TLB

/*
 * Use this to activate extra TLB profiling code
 * (currently not implemented)
 */
#undef CONFIG_PROFILE_TLB

#endif /* __ASM_MIPS_MIPSCONFIG_H */
