/*
 * Various ISA level dependant constants.
 * Most of the following constants reflect the different layout
 * of Coprocessor 0 registers.
 *
 * Copyright (c) 1998 Harald Koerfgen
 *
 * $Id: $
 */
#include <asm/sgidefs.h>

#ifndef __ASM_MIPS_ISADEP_H
#define __ASM_MIPS_ISADEP_H

#if (_MIPS_ISA == _MIPS_ISA_MIPS1)
/*
 * R2000 or R3000
 */

/*
 * kernel or user mode? (CP0_STATUS)
 */
#define KU_MASK 0x08
#define	KU_USER 0x08
#define KU_KERN 0x00

#else
/*
 * R6000 or higher
 */

/*
 * kernel or user mode?
 */
#define KU_MASK 0x18
#define	KU_USER 0x10
#define KU_KERN 0x00

#endif /* (_MIPS_ISA == _MIPS_ISA_MIPS1) */

#endif /* __ASM_MIPS_ISADEP_H */
