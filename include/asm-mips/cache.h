/*
 * Defines related to MIPS caches.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * (C) Copyright 1996 by Ralf Baechle
 */
#ifndef	__ASM_MIPS_CACHE
#define	__ASM_MIPS_CACHE

/*
 * Cache Operations
 */
#define Index_Invalidate_I      0x00
#define Index_Writeback_Inv_D   0x01
#define Index_Invalidate_SI     0x02
#define Index_Writeback_Inv_SD  0x03
#define Index_Load_Tag_I	0x04
#define Index_Load_Tag_D	0x05
#define Index_Load_Tag_SI	0x06
#define Index_Load_Tag_SD	0x07
#define Index_Store_Tag_I	0x08
#define Index_Store_Tag_D	0x09
#define Index_Store_Tag_SI	0x0A
#define Index_Store_Tag_SD	0x0B
#define Create_Dirty_Excl_D	0x0d
#define Create_Dirty_Excl_SD	0x0f
#define Hit_Invalidate_I	0x10
#define Hit_Invalidate_D	0x11
#define Hit_Invalidate_SI	0x12
#define Hit_Invalidate_SD	0x13
#define Fill			0x14
#define Hit_Writeback_Inv_D	0x15
					/* 0x16 is unused */
#define Hit_Writeback_Inv_SD	0x17
#define Hit_Writeback_I		0x18
#define Hit_Writeback_D		0x19
					/* 0x1a is unused */
#define Hit_Writeback_SD	0x1b
					/* 0x1c is unused */
					/* 0x1e is unused */
#define Hit_Set_Virtual_SI	0x1e
#define Hit_Set_Virtual_SD	0x1f

/*
 * Flags for internal cacheflush call
 * FIXME: CF_DMA is not implemented yet.
 */
#define	CF_ICACHE	(1<<0)		/* flush instruction cache        */
#define	CF_DCACHE	(1<<1)		/* writeback and flush data cache */
#define	CF_BCACHE	(CF_ICACHE|CF_DCACHE)	/* writeback and flush data cache */
#define CF_VIRTUAL	(1<<2)		/* operate only on virtual caches */
#define CF_PHYSICAL	(1<<4)		/* operate only on physical caches */
#define CF_ALL		(CF_VIRTUAL|CF_PHYSICAL)/* work on all cache types */
#define CF_DMA		(1<<5)		/* Do DMA cache maintenance */

#ifndef __LANGUAGE_ASSEMBLY__

#include <linux/linkage.h>

extern void (*mips_cache_init)(void);
extern asmlinkage void (*cacheflush)(unsigned long addr, unsigned long nbytes, unsigned int flags);

#endif /* !__LANGUAGE_ASSEMBLY__ */
#endif	/* __ASM_MIPS_CACHE */
