/*
 * Kevin D. Kissell, kevink@mips.com and Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
 *
 * This program is free software; you can distribute it and/or modify it
 * under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * MIPS32 CPU variant specific MMU/Cache routines.
 */
#include <asm/bootinfo.h>
#include <asm/cacheops.h>
#include <asm/cpu.h>
#include <asm/page.h>

extern int dc_lsize, ic_lsize, sc_lsize;

/*
 * Zero an entire page.
 */

void mips32_clear_page_dc(unsigned long page)
{
	unsigned long i;

        if (mips_cpu.options & MIPS_CPU_CACHE_CDEX) {
	        for (i=page; i<page+PAGE_SIZE; i+=dc_lsize) {
		        __asm__ __volatile__(
			        ".set\tnoreorder\n\t"
				".set\tnoat\n\t"
				".set\tmips3\n\t"
				"cache\t%2,(%0)\n\t"
				".set\tmips0\n\t"
				".set\tat\n\t"
				".set\treorder"
				:"=r" (i)
				:"0" (i),
				"I" (Create_Dirty_Excl_D));
		}
	}
	for (i=page; i<page+PAGE_SIZE; i+=4)
	        *(unsigned long *)(i) = 0;
}

void mips32_clear_page_sc(unsigned long page)
{
	unsigned long i;

        if (mips_cpu.options & MIPS_CPU_CACHE_CDEX) {
	        for (i=page; i<page+PAGE_SIZE; i+=sc_lsize) {
		        __asm__ __volatile__(
				".set\tnoreorder\n\t"
				".set\tnoat\n\t"
				".set\tmips3\n\t"
				"cache\t%2,(%0)\n\t"
				".set\tmips0\n\t"
				".set\tat\n\t"
				".set\treorder"
				:"=r" (i)
				:"0" (i),
				"I" (Create_Dirty_Excl_SD));
		}
	}
	for (i=page; i<page+PAGE_SIZE; i+=4)
	        *(unsigned long *)(i) = 0;
}

void mips32_copy_page_dc(unsigned long to, unsigned long from)
{
	unsigned long i;

        if (mips_cpu.options & MIPS_CPU_CACHE_CDEX) {
	        for (i=to; i<to+PAGE_SIZE; i+=dc_lsize) {
		        __asm__ __volatile__(
			        ".set\tnoreorder\n\t"
				".set\tnoat\n\t"
				".set\tmips3\n\t"
				"cache\t%2,(%0)\n\t"
				".set\tmips0\n\t"
				".set\tat\n\t"
				".set\treorder"
				:"=r" (i)
				:"0" (i),
				"I" (Create_Dirty_Excl_D));
		}
	}
	for (i=0; i<PAGE_SIZE; i+=4)
	        *(unsigned long *)(to+i) = *(unsigned long *)(from+i);
}

void mips32_copy_page_sc(unsigned long to, unsigned long from)
{
	unsigned long i;

        if (mips_cpu.options & MIPS_CPU_CACHE_CDEX) {
	        for (i=to; i<to+PAGE_SIZE; i+=sc_lsize) {
		        __asm__ __volatile__(
				".set\tnoreorder\n\t"
				".set\tnoat\n\t"
				".set\tmips3\n\t"
				"cache\t%2,(%0)\n\t"
				".set\tmips0\n\t"
				".set\tat\n\t"
				".set\treorder"
				:"=r" (i)
				:"0" (i),
				"I" (Create_Dirty_Excl_SD));
		}
	}
	for (i=0; i<PAGE_SIZE; i+=4)
	        *(unsigned long *)(to+i) = *(unsigned long *)(from+i);
}
