/*
 * Written by Kanoj Sarcar (kanoj@sgi.com) Aug 99
 * Rewritten for Linux 2.6 by Christoph Hellwig (hch@lst.de) Jan 2004
 */
#ifndef _ASM_MMZONE_H_
#define _ASM_MMZONE_H_

#include <mmzone.h>

#define kvaddr_to_nid(kvaddr)	pa_to_nid(__pa(kvaddr))
#define pfn_to_nid(pfn)		pa_to_nid((pfn) << PAGE_SHIFT)
#define pfn_valid(pfn)		((pfn) < num_physpages)

#define pfn_to_page(pfn)					\
({								\
 	unsigned long __pfn = (pfn);				\
	pg_data_t *__pg = NODE_DATA(pfn_to_nid(__pfn));		\
	__pg->node_mem_map + (__pfn - __pg->node_start_pfn);	\
})

#define page_to_pfn(p)						\
({								\
	struct page *__p = (p);					\
	struct zone *__z = page_zone(__p);			\
	((__p - __z->zone_mem_map) + __z->zone_start_pfn);	\
})

/* XXX(hch): better get rid of this thingy, it's rather fragile.. */
static inline int kern_addr_valid(unsigned long kvaddr)
{
	unsigned node = kvaddr_to_nid(kvaddr);
	unsigned long bit;
	pg_data_t *pg;

	if (node >= numnodes)
		return 0;
	pg = NODE_DATA(node);
	bit = (__pa(kvaddr) >> PAGE_SHIFT) - pg->node_start_pfn;

	return test_bit(bit, pg->valid_addr_bitmap);
}

#endif /* _ASM_MMZONE_H_ */
