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

#ifndef PCI_DMA_DEBUG
#define PCI_DMA_DEBUG 0
#endif

#if PCI_DMA_DEBUG>=1
#define DPRINTK(str,args...) printk (KERN_DEBUG "MACEPCI: %s: " str, __FUNCTION__ , ## args)
#else
#define DPRINTK(str,args...)
#endif

#define mips_wbflush()  __asm__ __volatile__ ("sync" : : : "memory")
/*#define mace_inv_read_buffers mips_wbflush*/

void *pci_alloc_consistent(struct pci_dev *hwdev, size_t size,
			   dma_addr_t * dma_handle)
{
	void *ret;
	int gfp = GFP_ATOMIC;

	if (hwdev != NULL && hwdev->dma_mask != 0xffffffff)
		gfp |= GFP_DMA;
	ret = (void *) __get_free_pages(gfp, get_order(size));

	if (ret != NULL) {
		memset(ret, 0, size);
		dma_cache_wback_inv((unsigned long) ret, size);
		*dma_handle = ( __pa (ret));
		DPRINTK("pci_alloc_consistent: addr=%016lx; dma_handle=%08x\n",(u64)KSEG1ADDR(ret),*dma_handle);
		return KSEG1ADDR(ret);
	}
	DPRINTK("pci_alloc_consistent2: addr=%016lx; dma_handle=%08x\n",(u64)KSEG1ADDR(ret),*dma_handle);
	return NULL;
}

void pci_free_consistent(struct pci_dev *hwdev, size_t size,
			 void *vaddr, dma_addr_t dma_handle)
{
	DPRINTK("pci_free_consistent: addr=%016lx; dma_handle=%08x\n",(u64)KSEG0ADDR(vaddr),dma_handle);
	free_pages((unsigned long) KSEG0ADDR(vaddr), get_order(size));
}

dma_addr_t pci_map_single(struct pci_dev *hwdev, void *ptr,
					size_t size, int direction)
{
	if (direction == PCI_DMA_NONE)
		BUG();

	DPRINTK("pci_map_single: addr=%016lx; dma_handle=%08x\n",(u64)ptr,__pa(ptr));
	mips_wbflush();
	dma_cache_wback_inv((unsigned long)ptr, size);
/*	if(direction==PCI_DMA_TODEVICE)
		mace_inv_read_buffers();*/
	return (__pa(ptr));
}

/*
 * Unmap a single streaming mode DMA translation.  The dma_addr and size
 * must match what was provided for in a previous pci_map_single call.  All
 * other usages are undefined.
 *
 * After this call, reads by the cpu to the buffer are guarenteed to see
 * whatever the device wrote there.
 */
void pci_unmap_single(struct pci_dev *hwdev, dma_addr_t dma_addr,
				    size_t size, int direction)
{
	if (direction == PCI_DMA_NONE)
		BUG();
	DPRINTK("pci_unmap_single\n");
	/* Nothing to do */
}

/*
 * Map a set of buffers described by scatterlist in streaming
 * mode for DMA.  This is the scather-gather version of the
 * above pci_map_single interface.  Here the scatter gather list
 * elements are each tagged with the appropriate dma address
 * and length.  They are obtained via sg_dma_{address,length}(SG).
 *
 * NOTE: An implementation may be able to use a smaller number of
 *       DMA address/length pairs than there are SG table elements.
 *       (for example via virtual mapping capabilities)
 *       The routine returns the number of addr/length pairs actually
 *       used, at most nents.
 *
 * Device ownership issues as mentioned above for pci_map_single are
 * the same here.
 */
int pci_map_sg(struct pci_dev *hwdev, struct scatterlist *sg,
			     int nents, int direction)
{
	int i;

	if (direction == PCI_DMA_NONE)
		BUG();

/*	if(direction==PCI_DMA_TODEVICE)
		mace_inv_read_buffers();*/
	DPRINTK("pci_map_sg\n");
	/* Make sure that gcc doesn't leave the empty loop body.  */
	for (i = 0; i < nents; i++, sg++) {
		mips_wbflush();
		dma_cache_wback_inv((unsigned long)sg->address, sg->length);
		sg->address = (char *)(__pa(sg->address));
	}

	return nents;
}

/*
 * Unmap a set of streaming mode DMA translations.
 * Again, cpu read rules concerning calls here are the same as for
 * pci_unmap_single() above.
 */
void pci_unmap_sg(struct pci_dev *hwdev, struct scatterlist *sg,
				int nents, int direction)
{
	if (direction == PCI_DMA_NONE)
		BUG();
	DPRINTK("pci_unmap_sg\n");
	/* Nothing to do */
}

/*
 * Make physical memory consistent for a single
 * streaming mode DMA translation after a transfer.
 *
 * If you perform a pci_map_single() but wish to interrogate the
 * buffer using the cpu, yet do not wish to teardown the PCI dma
 * mapping, you must call this function before doing so.  At the
 * next point you give the PCI dma address back to the card, the
 * device again owns the buffer.
 */
void pci_dma_sync_single(struct pci_dev *hwdev,
				       dma_addr_t dma_handle,
				       size_t size, int direction)
{
	if (direction == PCI_DMA_NONE)
		BUG();
	DPRINTK("pci_dma_sync_single: addr=%016lx dma_handle=%08x\n",__va(dma_handle),dma_handle);
	mips_wbflush();
	dma_cache_wback_inv((unsigned long)__va(dma_handle), size);
/*	if(direction==PCI_DMA_TODEVICE)
		mace_inv_read_buffers();*/
}

/*
 * Make physical memory consistent for a set of streaming
 * mode DMA translations after a transfer.
 *
 * The same as pci_dma_sync_single but for a scatter-gather list,
 * same rules and usage.
 */
void pci_dma_sync_sg(struct pci_dev *hwdev,
				   struct scatterlist *sg,
				   int nelems, int direction)
{
	int i;
	if (direction == PCI_DMA_NONE)
		BUG();
	DPRINTK("pci_dma_sync_sg\n");
	/*  Make sure that gcc doesn't leave the empty loop body.  */
	for (i = 0; i < nelems; i++, sg++){
		mips_wbflush();
		dma_cache_wback_inv((unsigned long)__va(sg->address), sg->length);
	}
/*	if(direction==PCI_DMA_TODEVICE)
		mace_inv_read_buffers();*/
}
