/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1998, 1999 by Ralf Baechle
 */
#ifndef _ASM_CACHE_H
#define _ASM_CACHE_H

/* bytes per L1 cache line */
#define        L1_CACHE_BYTES  32      /* a guess */

#define        L1_CACHE_ALIGN(x)       (((x)+(L1_CACHE_BYTES-1))&~(L1_CACHE_BYTES-1))

#define        SMP_CACHE_BYTES L1_CACHE_BYTES

#endif /* _ASM_CACHE_H */
