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

#undef DEBUG_MACE_PCI
#ifdef DEBUG_MACE_PCI
# define DPRINTK(fmt,args...) printk(KERN_DEBUG "%s: %d:" fmt "\n", __FILE__, __LINE__, ##args);
#else
# define DPRINTK(fmt...)
#endif

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

#define mkaddr(_devfn, _reg) \
((((_devfn) & 0xffUL) << 8) | ((_reg) & 0xfcUL))

static int
mace_pci_read_config(struct pci_bus *bus, unsigned int devfn,
		     int reg, int size, u32 *val)
{
	chkslot(bus, devfn);
	mace_write_32(mkaddr(devfn, reg), MACEPCI_CONFIG_ADDR);
	switch (size) {
	case 1:
		*val = mace_read_8(MACEPCI_CONFIG_DATA + ((reg & 3UL) ^ 3UL));
		break;
	case 2:
		*val = mace_read_16(MACEPCI_CONFIG_DATA + ((reg & 2UL) ^ 2UL));
		break;
	case 4:
		*val = mace_read_32(MACEPCI_CONFIG_DATA);
		break;
	}

	DPRINTK("read%d: reg=%08x,val=%02x", size * 8, reg, *val);

	return PCIBIOS_SUCCESSFUL;
}

static int
mace_pci_write_config(struct pci_bus *bus, unsigned int devfn,
		      int reg, int size, u32 val)
{
	chkslot(bus, devfn);
	mace_write_32(mkaddr(devfn, reg), MACEPCI_CONFIG_ADDR);
	switch (size) {
	case 1:
		mace_write_8(val, MACEPCI_CONFIG_DATA + ((reg & 3UL) ^ 3UL));
		break;
	case 2:
		mace_write_16(val, MACEPCI_CONFIG_DATA + ((reg & 2UL) ^ 2UL));
		break;
	case 4:
		mace_write_32(val, MACEPCI_CONFIG_DATA);
		break;
	}

	DPRINTK("write%d: reg=%08x,val=%02x", size * 8, reg, val);

	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops mace_pci_ops = {
	.read = mace_pci_read_config,
	.write = mace_pci_write_config,
};
