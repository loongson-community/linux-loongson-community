/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000, 2001 Keith M Wesolowski
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <asm/pci.h>
#include <asm/ip32/mace.h>
#include <asm/ip32/crime.h>
#include <asm/ip32/ip32_ints.h>
#include <linux/delay.h>

#undef DEBUG_MACE_PCI

/*
 * O2 has up to 5 PCI devices connected into the MACE bridge.  The device
 * map looks like this:
 *
 * 0  aic7xxx 0
 * 1  aic7xxx 1
 * 2  expansion slot
 * 3  N/C
 * 4  N/C
 */

#define chkslot(_bus,_devfn)					\
do {							        \
	if ((_bus)->number > 0 || PCI_SLOT (_devfn) < 1	\
	    || PCI_SLOT (_devfn) > 3)			        \
		return PCIBIOS_DEVICE_NOT_FOUND;		\
} while (0)

#define mkaddr(_devfn, where) \
((((_devfn) & 0xffUL) << 8) | ((where) & 0xfcUL))

static int mace_pci_read_config(struct pci_bus *bus, unsigned int devfn,
			       int where, int size, u32 * val)
{
	switch (size) {
	case 1:
		*val = 0xff;
		chkslot(bus, devfn);
		mace_write_32(MACEPCI_CONFIG_ADDR, mkaddr(devfn, where));
		*val =
		    mace_read_8(MACEPCI_CONFIG_DATA +
				((where & 3UL) ^ 3UL));

		return PCIBIOS_SUCCESSFUL;

	case 2:
		*val = 0xffff;
		chkslot(bus, devfn);
		if (where & 1)
			return PCIBIOS_BAD_REGISTER_NUMBER;
		mace_write_32(MACEPCI_CONFIG_ADDR, mkaddr(devfn, where));
		*val =
		    mace_read_16(MACEPCI_CONFIG_DATA +
				 ((where & 2UL) ^ 2UL));

		return PCIBIOS_SUCCESSFUL;

	case 4:
		*val = 0xffffffff;
		chkslot(bus, devfn);
		if (where & 3)
			return PCIBIOS_BAD_REGISTER_NUMBER;
		mace_write_32(MACEPCI_CONFIG_ADDR, mkaddr(devfn, where));
		*val = mace_read_32(MACEPCI_CONFIG_DATA);

		return PCIBIOS_SUCCESSFUL;
	}

	return PCIBIOS_BAD_REGISTER_NUMBER;
}

static int mace_pci_write_config(struct pci_bus *bus, unsigned int devfn,
				int where, int size, u32 val)
{
	switch (size) {
	case 1:
		chkslot(bus, devfn);
		mace_write_32(MACEPCI_CONFIG_ADDR, mkaddr(devfn, where));
		mace_write_8(MACEPCI_CONFIG_DATA + ((where & 3UL) ^ 3UL),
			     val);

		return PCIBIOS_SUCCESSFUL;

	case 2:
		chkslot(bus, devfn);
		if (where & 1)
			return PCIBIOS_BAD_REGISTER_NUMBER;
		mace_write_32(MACEPCI_CONFIG_ADDR, mkaddr(devfn, where));
		mace_write_16(MACEPCI_CONFIG_DATA + ((where & 2UL) ^ 2UL),
			      val);

		return PCIBIOS_SUCCESSFUL;

	case 4:
		chkslot(bus, devfn);
		if (where & 3)
			return PCIBIOS_BAD_REGISTER_NUMBER;
		mace_write_32(MACEPCI_CONFIG_ADDR, mkaddr(devfn, where));
		mace_write_32(MACEPCI_CONFIG_DATA, val);

		return PCIBIOS_SUCCESSFUL;
	}

	return PCIBIOS_BAD_REGISTER_NUMBER;
}

struct pci_ops mace_pci_ops = {
	.read = mace_pci_read_config,
	.write = mace_pci_write_config,
};
