/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000 by Ralf Baechle
 * Copyright (C) 2000 Silicon Graphics, Inc.
 */
#ifndef _ASM_HIGHMEM_H
#define _ASM_HIGHMEM_H

#include <linux/init.h>
#include <asm/pgtable.h>

/* declarations for highmem.c */
extern unsigned long highstart_pfn, highend_pfn;

extern pte_t *kmap_pte;
extern pgprot_t kmap_prot;

#define kmap_init()		do { } while(0)
#define kmap(page, type)	page_address(page)
#define kunmap(vaddr, type)	do { } while(0)
#define kmap_check()		do { } while(0)

#endif /* _ASM_HIGHMEM_H */
