/*
 * Copyright (C) 2000   Ani Joshi <ajoshi@unixbox.com>
 *
 *
 * Dynamic DMA mapping support.
 *
 * swiped from i386, and cloned for MIPS by Geert.
 *
 */
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/pci.h>

#include <asm/io.h>

/*
 * [jsun] We want to return non-cached area so that data can be consistent
 * Apparently on x86, this is not an issue because cache is automatically
 * invalidated.
 *
 * To make we are doing the right thing, I add some extra debug macros.
 */

void *pci_alloc_consistent(struct pci_dev *hwdev, size_t size,
			   dma_addr_t * dma_handle)
{
	void *ret;
	int gfp = GFP_ATOMIC;

	if (hwdev == NULL || hwdev->dma_mask != 0xffffffff)
		gfp |= GFP_DMA;
	ret = (void *) __get_free_pages(gfp, get_order(size));

	if (ret != NULL) {
		dma_cache_inv((unsigned long) ret, size);
		*dma_handle = virt_to_bus(ret);
	}
	ret = (void *) ((unsigned long) ret | 0x20000000);
	return ret;
}

void pci_free_consistent(struct pci_dev *hwdev, size_t size,
			 void *vaddr, dma_addr_t dma_handle)
{
	vaddr = (void *) ((unsigned long) vaddr & ~0x20000000);
	free_pages((unsigned long) vaddr, get_order(size));
}
