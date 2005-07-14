/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2005 Embedded Alley Solutions, Inc
 */
#ifndef __ASM_MACH_GENERIC_KERNEL_ENTRY_H
#define __ASM_MACH_GENERIC_KERNEL_ENTRY_H

/* Intentionally empty macro, used in head.S. Override in 
 * arch/mips/mach-xxx/kernel-entry-init.h when necessary.
 */
.macro	kernel_entry_setup
.endm

#endif /* __ASM_MACH_GENERIC_KERNEL_ENTRY_H */
