/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1999, 2000, 01, 02, 03 by Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2001 Kanoj Sarcar
 */
#ifndef __ASM_MACH_IP27_IRQ_H
#define __ASM_MACH_IP27_IRQ_H

#include <asm/sn/arch.h>

/*
 * A hardwired interrupt number is completly stupid for this system - a
 * large configuration might have thousands if not tenthousands of
 * interrupts.
 */
#define NR_IRQS 256

/*
 * All the stuff that shouldn't be at all in irq.h starts here.
 */
/*
 * Number of levels in INT_PEND0. Can be set to 128 if we also
 * consider INT_PEND1.
 */
#define PERNODE_LEVELS	64

extern int node_level_to_irq[MAX_COMPACT_NODES][PERNODE_LEVELS];

/*
 * we need to map irq's up to at least bit 7 of the INT_MASK0_A register
 * since bits 0-6 are pre-allocated for other purposes.
 */
#define LEAST_LEVEL	7
#define FAST_IRQ_TO_LEVEL(i)	((i) + LEAST_LEVEL)
#define LEVEL_TO_IRQ(c, l) \
			(node_level_to_irq[CPUID_TO_COMPACT_NODEID(c)][(l)])

#endif /* __ASM_MACH_IP27_IRQ_H */
