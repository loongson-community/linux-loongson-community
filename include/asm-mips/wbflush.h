/*
 * Header file for using the wbflush routine
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 1998 Harald Koerfgen
 *
 * $Id: $
 */
#ifndef __ASM_MIPS_WBFLUSH_H
#define __ASM_MIPS_WBFLUSH_H

#include <asm/sgidefs.h>

#if (_MIPS_ISA == _MIPS_ISA_MIPS1)
/*
 * R2000 or R3000
 */
extern void (*__wbflush) (void);

#define wbflush() __wbflush()

#else
/*
 * we don't need no stinkin' wbflush
 */

#define wbflush()

#endif

#endif /* __ASM_MIPS_WBFLUSH_H */
