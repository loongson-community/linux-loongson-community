/*
 * Cobalt Qube/Raq PCI support
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995, 1996, 1997, 2002 by Ralf Baechle
 * Copyright (C) 2001, 2002, 2003 by Liam Davies (ldavies@agile.tv)
 */
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <asm/pci.h>
#include <asm/io.h>
#include <asm/gt64120.h>

#include <asm/cobalt/cobalt.h>

int cobalt_board_id;

static inline int pci_range_ck(struct pci_bus *bus, unsigned int devfn)
{
	if ((bus->number == 0)
	    && ((PCI_SLOT(devfn) == 0)
		|| ((PCI_SLOT(devfn) > 6)
		    && (PCI_SLOT(devfn) <= 12))))
		return 0;	/* OK device number  */

	return -1;		/* NOT ok device number */
}

#define PCI_CFG_SET(devfn,where) \
       GALILEO_OUTL((0x80000000 | (PCI_SLOT (devfn) << 11) | \
                           (PCI_FUNC (devfn) << 8) | (where)), \
                           GT_PCI0_CFGADDR_OFS)


static int qube_pci_read_config(struct pci_bus *bus, unsigned int devfn,
				int where, int size, u32 * val)
{
	switch (size) {
	case 4:
		if (where & 0x3)
			return PCIBIOS_BAD_REGISTER_NUMBER;
		if (pci_range_ck(bus, devfn)) {
			*val = 0xFFFFFFFF;
			return PCIBIOS_DEVICE_NOT_FOUND;
		}
		PCI_CFG_SET(devfn, where);
		*val = GALILEO_INL(GT_PCI0_CFGDATA_OFS);
		return PCIBIOS_SUCCESSFUL;

	case 2:
		if (where & 0x1)
			return PCIBIOS_BAD_REGISTER_NUMBER;
		if (pci_range_ck(bus, devfn)) {
			*val = 0xffff;
			return PCIBIOS_DEVICE_NOT_FOUND;
		}
		PCI_CFG_SET(devfn, (where & ~0x3));
		*val = GALILEO_INL(GT_PCI0_CFGDATA_OFS)
		    >> ((where & 3) * 8);
		return PCIBIOS_SUCCESSFUL;

	case 1:
		if (pci_range_ck(bus, devfn)) {
			*val = 0xff;
			return PCIBIOS_DEVICE_NOT_FOUND;
		}
		PCI_CFG_SET(devfn, (where & ~0x3));
		*val = GALILEO_INL(GT_PCI0_CFGDATA_OFS)
		    >> ((where & 3) * 8);
		return PCIBIOS_SUCCESSFUL;
	}

	return PCIBIOS_BAD_REGISTER_NUMBER;
}

static int qube_pci_write_config(struct pci_bus *bus, unsigned int devfn,
				 int where, int size, u32 val)
{
	u32 tmp;

	switch (size) {
	case 4:
		if (where & 0x3)
			return PCIBIOS_BAD_REGISTER_NUMBER;
		if (pci_range_ck(bus, devfn))
			return PCIBIOS_DEVICE_NOT_FOUND;
		PCI_CFG_SET(devfn, where);
		GALILEO_OUTL(val, GT_PCI0_CFGDATA_OFS);

		return PCIBIOS_SUCCESSFUL;

	case 2:
		if (where & 0x1)
			return PCIBIOS_BAD_REGISTER_NUMBER;
		if (pci_range_ck(bus, devfn))
			return PCIBIOS_DEVICE_NOT_FOUND;
		PCI_CFG_SET(devfn, (where & ~0x3));
		tmp = GALILEO_INL(GT_PCI0_CFGDATA_OFS);
		tmp &= ~(0xffff << ((where & 0x3) * 8));
		tmp |= (val << ((where & 0x3) * 8));
		GALILEO_OUTL(tmp, GT_PCI0_CFGDATA_OFS);

		return PCIBIOS_SUCCESSFUL;

	case 1:

		if (pci_range_ck(bus, devfn))
			return PCIBIOS_DEVICE_NOT_FOUND;
		PCI_CFG_SET(devfn, (where & ~0x3));
		tmp = GALILEO_INL(GT_PCI0_CFGDATA_OFS);
		tmp &= ~(0xff << ((where & 0x3) * 8));
		tmp |= (val << ((where & 0x3) * 8));
		GALILEO_OUTL(tmp, GT_PCI0_CFGDATA_OFS);

		return PCIBIOS_SUCCESSFUL;
	}

	return PCIBIOS_BAD_REGISTER_NUMBER;
}

struct pci_ops qube_pci_ops = {
	.read = qube_pci_read_config,
	.write = qube_pci_write_config,
};

static int __init pcibios_init(void)
{
	unsigned int devfn = PCI_DEVFN(COBALT_PCICONF_VIA, 0);

	printk("PCI: Probing PCI hardware\n");

	/* Read the cobalt id register out of the PCI config space */
	PCI_CFG_SET(devfn, (VIA_COBALT_BRD_ID_REG & ~0x3));
	cobalt_board_id = GALILEO_INL(GT_PCI0_CFGDATA_OFS)
	    >> ((VIA_COBALT_BRD_ID_REG & 3) * 8);
	cobalt_board_id = VIA_COBALT_BRD_REG_to_ID(cobalt_board_id);

	printk("Cobalt Board ID: %d\n", cobalt_board_id);

	ioport_resource.start = 0x00000000;
	ioport_resource.end = 0x0fffffff;

	iomem_resource.start = 0x01000000;
	iomem_resource.end = 0xffffffff;

	pci_scan_bus(0, &qube_pci_ops, NULL);

	return 0;
}

subsys_initcall(pcibios_init);
