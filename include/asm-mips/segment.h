/*
 * include/asm-mips/segment.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 by Ralf Baechle
 *
 */
#ifndef __ASM_MIPS_SEGMENT_H
#define __ASM_MIPS_SEGMENT_H

#ifndef __LANGUAGE_ASSEMBLY__
/*
 * Uh, these should become the main single-value transfer routines..
 * They automatically use the right size if we just have the right
 * pointer type..
 */
#define put_user(x,ptr) __put_user((unsigned long)(x),(ptr),sizeof(*(ptr)))
#define get_user(ptr) __get_user((ptr),sizeof(*(ptr)))

/*
 * This is a silly but good way to make sure that
 * the __put_user function is indeed always optimized,
 * and that we use the correct sizes..
 */
extern int bad_user_access_length(void);

/* I should make this use unaligned transfers etc.. */
static inline void __put_user(unsigned long x, void * y, int size)
{
	switch (size) {
		case 1:
			*(char *) y = x;
			break;
		case 2:
			*(short *) y = x;
			break;
		case 4:
			*(int *) y = x;
			break;
		case 8:
			*(long *) y = x;
			break;
		default:
			bad_user_access_length();
	}
}

/* I should make this use unaligned transfers etc.. */
static inline unsigned long __get_user(void * y, int size)
{
	switch (size) {
		case 1:
			return *(unsigned char *) y;
		case 2:
			return *(unsigned short *) y;
		case 4:
			return *(unsigned int *) y;
		case 8:
			return *(unsigned long *) y;
		default:
			return bad_user_access_length();
	}
}
#endif /* __LANGUAGE_ASSEMBLY__ */

/*
 * Memory segments (32bit kernel mode addresses)
 */
#define KUSEG                   0x00000000
#define KSEG0                   0x80000000
#define KSEG1                   0xa0000000
#define KSEG2                   0xc0000000
#define KSEG3                   0xe0000000

/*
 * Returns the kernel segment base of a given address
 */
#define KSEGX(a)                (a & 0xe0000000)

/*
 * Returns the physical address of a KSEG0/KSEG1 address
 */

#define PHYSADDR(a)		((unsigned long)a & 0x1fffffff)

/*
 * Map an address to a certain kernel segment
 */

#define KSEG0ADDR(a)		(((unsigned long)a & 0x1fffffff) | KSEG0)
#define KSEG1ADDR(a)		(((unsigned long)a & 0x1fffffff) | KSEG1)
#define KSEG2ADDR(a)		(((unsigned long)a & 0x1fffffff) | KSEG2)
#define KSEG3ADDR(a)		(((unsigned long)a & 0x1fffffff) | KSEG3)


/*
 * Memory segments (64bit kernel mode addresses)
 */
#define XKUSEG                  0x0000 0000 0000 0000
#define XKSSEG                  0x4000 0000 0000 0000
#define XKPHYS                  0x8000 0000 0000 0000
#define XKSEG                   0xc000 0000 0000 0000
#define CKSEG0                  0xffff ffff 8000 0000
#define CKSEG1                  0xffff ffff a000 0000
#define CKSSEG                  0xffff ffff c000 0000
#define CKSEG3                  0xffff ffff e000 0000

#ifndef __LANGUAGE_ASSEMBLY__
/*
 * These are deprecated
 */

#define get_fs_byte(addr) get_user_byte((char *)(addr))
static inline unsigned char get_user_byte(const char *addr)
{
	return *addr;
}

/*
 * Beware: the xxx_fs_word functions work on 16bit words!
 */
#define get_fs_word(addr) get_user_word((short *)(addr))
static inline unsigned short get_user_word(const short *addr)
{
	return *addr;
}

#define get_fs_long(addr) get_user_long((int *)(addr))
static inline unsigned long get_user_long(const int *addr)
{
	return *addr;
}

#define get_fs_dlong(addr) get_user_dlong((long long *)(addr))
static inline unsigned long get_user_dlong(const long long *addr)
{
	return *addr;
}

#define put_fs_byte(x,addr) put_user_byte((x),(char *)(addr))
static inline void put_user_byte(char val,char *addr)
{
	*addr = val;
}

#define put_fs_word(x,addr) put_user_word((x),(short *)(addr))
static inline void put_user_word(short val,short * addr)
{
	*addr = val;
}

#define put_fs_long(x,addr) put_user_long((x),(int *)(addr))
static inline void put_user_long(unsigned long val,int * addr)
{
	*addr = val;
}

#define put_fs_dlong(x,addr) put_user_dlong((x),(int *)(addr))
static inline void put_user_dlong(unsigned long val,long long * addr)
{
	*addr = val;
}

#define memcpy_fromfs(to, from, n) memcpy((to),(from),(n))

#define memcpy_tofs(to, from, n) memcpy((to),(from),(n))

/*
 * For segmented architectures, these are used to specify which segment
 * to use for the above functions.
 *
 * MIPS is not segmented, so these are just dummies.
 */

#define KERNEL_DS 0
#define USER_DS 1

static inline unsigned long get_fs(void)
{
	return 0;
}

static inline unsigned long get_ds(void)
{
	return 0;
}

static inline void set_fs(unsigned long val)
{
}

#endif /* !__LANGUAGE_ASSEMBLY__ */

#endif /* __ASM_MIPS_SEGMENT_H */
