/*
 * Switch a MMU context.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by Ralf Baechle
 */
#ifndef __ASM_MIPS_MMU_CONTEXT_H
#define __ASM_MIPS_MMU_CONTEXT_H

/*
 * Get a new mmu context.  For MIPS we use resume() to change the
 * PID (R3000)/ASID (R4000).  Well, we will ...
 */
extern __inline__ void
get_mmu_context(struct task_struct *p)
{
}

#endif /* __ASM_MIPS_MMU_CONTEXT_H */
