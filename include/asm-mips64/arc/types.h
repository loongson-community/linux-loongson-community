/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright 1999 Ralf Baechle (ralf@gnu.org)
 * Copyright 1999 Silicon Graphics, Inc.
 */
#ifndef _ASM_ARC_TYPES_H
#define _ASM_ARC_TYPES_H

#include <linux/config.h>

#ifdef CONFIG_ARC32

typedef char		CHAR;
typedef short		SHORT;
typedef long		LARGE_INTEGER __attribute__ ((__mode__ (__DI__)));
typedef	long		LONG __attribute__ ((__mode__ (__SI__)));
typedef unsigned char	UCHAR;
typedef unsigned short	USHORT;
typedef unsigned long	ULONG __attribute__ ((__mode__ (__SI__)));
typedef void		VOID;

#endif /* CONFIG_ARC32 */

#ifdef CONFIG_ARC64

typedef char		CHAR;
typedef short		SHORT;
typedef long		LARGE_INTEGER __attribute__ (__mode__ (__DI__));
typedef	long		LONG __attribute__ (__mode__ (__DI__));
typedef unsigned char	UCHAR;
typedef unsigned short	USHORT;
typedef unsigned long	ULONG __attribute__ (__mode__ (__DI__));
typedef void		VOID;

#endif /* CONFIG_ARC64  */

#endif /* _ASM_ARC_TYPES_H */
