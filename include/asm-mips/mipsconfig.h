/*
 * include/asm-mips/mipsconfig.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995, 1996 by Ralf Baechle
 *
 * This file contains constant definitions and some MIPS specific debug and
 * other compile time switches.
 */

#ifndef __ASM_MIPS_MIPSCONFIG_H
#define __ASM_MIPS_MIPSCONFIG_H

/*
 * This is the virtual address to which all ports are being mapped.
 * Must be a value that can be load with a lui instruction.
 */
#define PORT_BASE_SNI		0xb4000000
#define PORT_BASE_RPC44		0xe2000000
#define PORT_BASE_TYNE		0xe2000000
#define PORT_BASE_JAZZ		0xe2000000
#ifndef PORT_BASE
#if !defined (__LANGUAGE_ASSEMBLY__)
extern unsigned long port_base;
#endif
#define PORT_BASE port_base
#endif

/*
 * Pagetables are 4MB mapped at 0xe4000000
 * Must be a value that can be loaded with a single instruction.
 * The xtlb exception handler assumes that bit 30 and 31 of TLBMAP
 * are the same.  This is true for all KSEG2 and KSEG3 addresses and
 * will therefore not a problem.
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

#endif /* __ASM_MIPS_MIPSCONFIG_H */
