/*
 * include/asm-mips/types.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 by Waldorf GMBH
 * written by Ralf Baechle
 */
#ifndef __ASM_MIPS_TYPES_H
#define __ASM_MIPS_TYPES_H

#ifndef _SIZE_T
#define _SIZE_T
typedef __SIZE_TYPE__ size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef __SSIZE_TYPE__ ssize_t;
#endif

#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef __PTRDIFF_TYPE__ ptrdiff_t;
#endif

#ifndef _TIME_T
#define _TIME_T
typedef long time_t;
#endif

#ifndef _CLOCK_T
#define _CLOCK_T
typedef long clock_t;
#endif

/*
 * __xx is ok: it doesn't pollute the POSIX namespace. Use these in the
 * header files exported to user space
 */

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

#if ((~0UL) == 0xffffffff)

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#endif
  
#else
  
typedef __signed__ long __s64;
typedef unsigned long __u64;

#endif

/*
 * These aren't exported outside the kernel to avoid name space clashes
 */
#ifdef __KERNEL__

typedef __signed char s8;
typedef unsigned char u8;

typedef __signed short s16;
typedef unsigned short u16;

typedef __signed int s32;
typedef unsigned int u32;

#if ((~0UL) == 0xffffffff)

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
typedef __signed__ long long s64;
typedef unsigned long long u64;
#endif
  
#else
  
typedef __signed__ long s64;
typedef unsigned long u64;

#endif

#endif /* __KERNEL__ */

typedef __s32 pid_t;
typedef __s32 uid_t;
typedef __s32 gid_t;
typedef __u32 dev_t;
typedef __u32 ino_t;
typedef __u32 mode_t;
typedef __u32 umode_t;
typedef __u32 nlink_t;
typedef long daddr_t;
typedef long off_t;

#if 0
/*
 * These definitions double the definitions from <gnu/types.h>.
 */
#undef  __FDELT
#define __FDELT(d)      ((d) / __NFDBITS)
#undef  __FDMASK
#define __FDMASK(d)     (1 << ((d) % __NFDBITS))
#undef  __FD_SET
#define __FD_SET(d, set)        ((set)->fds_bits[__FDELT(d)] |= __FDMASK(d))
#undef  __FD_CLR
#define __FD_CLR(d, set)        ((set)->fds_bits[__FDELT(d)] &= ~__FDMASK(d))
#undef  __FD_ISSET
#define __FD_ISSET(d, set)      ((set)->fds_bits[__FDELT(d)] & __FDMASK(d))
#undef  __FD_ZERO
#define __FD_ZERO(fdsetp) (memset (fdsetp, 0, sizeof(*(fd_set *)fdsetp)))
#endif

#endif /* __ASM_MIPS_TYPES_H */
