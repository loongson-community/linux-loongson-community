/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1997, 1998, 1999 Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */
#ifndef _ASM_CACHE_H
#define _ASM_CACHE_H

#include <linux/config.h>

#ifndef __ASSEMBLY__
/*
 * Descriptor for a cache
 */
struct cache_desc {
	int linesz;
	int sets;
	int ways;
	int flags;	/* Details like write thru/back, coherent, etc. */
};
#endif /* !__ASSEMBLY__ */

/* bytes per L1 cache line */
#define L1_CACHE_BYTES		(1 << CONFIG_L1_CACHE_SHIFT)

#endif /* _ASM_CACHE_H */
