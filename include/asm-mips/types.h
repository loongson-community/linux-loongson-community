#ifndef _ASM_MIPS_TYPES_H
#define _ASM_MIPS_TYPES_H

/*
 * These aren't exported outside the kernel to avoid name space clashes
 */
#ifdef __KERNEL__

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed long s32;
typedef unsigned long u32;

typedef signed long long s64;
typedef unsigned long long u64;

#endif /* __KERNEL__ */

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

#endif /* _ASM_MIPS_TYPES_H */
