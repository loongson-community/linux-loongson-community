/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000  Ani Joshi <ajoshi@unixbox.com>
 * Copyright (C) 2000  Ralf Baechle <ralf@gnu.org>
 * swiped from i386, and cloned for MIPS by Geert, polished by Ralf.
 */
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/pci.h>

#include <asm/io.h>
#include <asm/addrspace.h>
#include <asm/ip32/mace.h>

void *pci_alloc_consistent(struct pci_dev *hwdev, size_t size,
			   dma_addr_t * dma_handle)
{
	void *ret;
	int gfp = GFP_ATOMIC;

	if (hwdev == NULL || hwdev->dma_mask != 0xffffffff)
		gfp |= GFP_DMA;
	ret = (void *) __get_free_pages(gfp, get_order(size));

	if (ret != NULL) {
		memset(ret, 0, size);
		dma_cache_wback_inv((unsigned long) ret, size);
		*dma_handle = KDM_TO_PHYS(ret);
	}
	return ret;
}

void pci_free_consistent(struct pci_dev *hwdev, size_t size,
			 void *vaddr, dma_addr_t dma_handle)
{
	free_pages((unsigned long) PHYS_TO_K0(vaddr), get_order(size));
}


/*
dma_addr_t pci_map_single(struct pci_dev *hwdev, void *ptr, size_t size,
                                 int direction)
{
	if (direction == PCI_DMA_NONE)
		BUG();
	
}
void pci_unmap_single(struct pci_dev *hwdev, dma_addr_t dma_addr,
                             size_t size, int direction)
{
	if (direction == PCI_DMA_NONE)
		BUG();
}
int pci_map_sg(struct pci_dev *hwdev, struct scatterlist *sg, int nents,
                      int direction)
{
	if (direction == PCI_DMA_NONE)
		BUG();
}
void pci_unmap_sg(struct pci_dev *hwdev, struct scatterlist *sg,
                         int nents, int direction)
{
	if (direction == PCI_DMA_NONE)
		BUG();
}
void pci_dma_sync_single(struct pci_dev *hwdev, dma_addr_t dma_handle,
                                size_t size, int direction)
{
	if (direction == PCI_DMA_NONE)
		BUG();
}
void pci_dma_sync_sg(struct pci_dev *hwdev, struct scatterlist *sg,
                            int nelems, int direction)
{
	if (direction == PCI_DMA_NONE)
		BUG();
}
*/
