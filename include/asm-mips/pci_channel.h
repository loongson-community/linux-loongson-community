#ifndef __ASM_PCI_CHANNEL_H
#define __ASM_PCI_CHANNEL_H

/*
 * This file essentially defines the interface between board
 * specific PCI code and MIPS common PCI code.  Should potentially put
 * into include/asm/pci.h file.
 */

#include <linux/ioport.h>
#include <linux/pci.h>

/*
 * Each pci channel is a top-level PCI bus seem by CPU.  A machine  with
 * multiple PCI channels may have multiple PCI host controllers or a
 * single controller supporting multiple channels.
 */
struct pci_controller {
	struct pci_controller *next;
	struct pci_bus *bus;

	struct pci_ops *pci_ops;
	struct resource *io_resource;
	struct resource *mem_resource;

	/* For compatibility with current (as of July 2003) pciutils
	   and XFree86. Eventually will be removed. */
	unsigned int need_domain_info;

	int first_devfn;
	int last_devfn;
};

/*
 * Used by boards to register their PCI interfaces before the actual scanning.
 */
extern struct pci_controller * alloc_pci_controller(void);
extern void register_pci_controller(struct pci_controller *hose);

/*
 * board supplied pci irq fixup routine
 */
extern void pcibios_fixup_irqs(void);

/*
 * board supplied pci fixup routines
 */
extern void pcibios_fixup_resources(struct pci_dev *dev);

#endif  /* __ASM_PCI_CHANNEL_H */
