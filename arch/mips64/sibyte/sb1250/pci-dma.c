/*
 * Copyright (C) 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* from: ip27-pci-dma.c
 *
 * Dynamic DMA mapping support.
 *
 * On the Origin there is dynamic DMA address translation for all PCI DMA.
 * However we don't use this facility yet but rely on the 2gb direct
 * mapped DMA window for PCI64.  So consistent alloc/free are merely page
 * allocation/freeing.  The rest of the dynamic DMA mapping interface is
 * implemented in <asm/pci.h>.  So this code will fail with more than
 * 2gb of memory.
 */
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/pci.h>

#include <asm/io.h>

void *pci_alloc_consistent(struct pci_dev *hwdev, size_t size,
			   dma_addr_t *dma_handle)
{
	void *ret;
	int gfp = GFP_ATOMIC;
	int order = get_order(size);

	if (hwdev == NULL || hwdev->dma_mask != 0xffffffff)
		gfp |= GFP_DMA;
	ret = (void *)__get_free_pages(gfp, order);

	if (ret != NULL) {
		memset(ret, 0, size);
		*dma_handle = (bus_to_baddr[hwdev->bus->number] | __pa(ret));
	}

	return ret;
}

void pci_free_consistent(struct pci_dev *hwdev, size_t size,
			 void *vaddr, dma_addr_t dma_handle)
{
	free_pages((unsigned long)vaddr, get_order(size));
}
