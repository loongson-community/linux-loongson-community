/* $Id: addrspace.h,v 1.2 1999/10/19 20:51:53 ralf Exp $
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1999 by Ralf Baechle
 * Copyright (C) 1999 by Silicon Graphics, Inc.
 */
#ifndef _ASM_ADDRSPACE_H
#define _ASM_ADDRSPACE_H

/*
 * Memory segments (32bit kernel mode addresses)
 */
#define KUSEG                   0x0000000000000000
#define KSEG0                   0xffffffff80000000
#define KSEG1                   0xffffffffa0000000
#define KSEG2                   0xffffffffc0000000
#define KSEG3                   0xffffffffe0000000

/*
 * Returns the kernel segment base of a given address
 */
#define KSEGX(a)                (((unsigned long)(a)) & 0xe0000000)

/*
 * Returns the physical address of a KSEG0/KSEG1 address
 */
#define PHYSADDR(a)		(((unsigned long)(a)) & 0x1fffffff)

/*
 * Map an address to a certain kernel segment
 */
#define KSEG0ADDR(a)		((__typeof__(a))(((unsigned long)(a) & 0x1fffffff) | KSEG0))
#define KSEG1ADDR(a)		((__typeof__(a))(((unsigned long)(a) & 0x1fffffff) | KSEG1))
#define KSEG2ADDR(a)		((__typeof__(a))(((unsigned long)(a) & 0x1fffffff) | KSEG2))
#define KSEG3ADDR(a)		((__typeof__(a))(((unsigned long)(a) & 0x1fffffff) | KSEG3))

/*
 * Memory segments (64bit kernel mode addresses)
 */
#define XKUSEG                  0x0000000000000000
#define XKSSEG                  0x4000000000000000
#define XKPHYS                  0x8000000000000000
#define XKSEG                   0xc000000000000000
#define CKSEG0                  0xffffffff80000000
#define CKSEG1                  0xffffffffa0000000
#define CKSSEG                  0xffffffffc0000000
#define CKSEG3                  0xffffffffe0000000

#endif /* _ASM_ADDRSPACE_H */
