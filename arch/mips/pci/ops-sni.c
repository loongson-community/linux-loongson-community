/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * SNI specific PCI support for RM200/RM300.
 *
 * Copyright (C) 1997 - 2000 Ralf Baechle
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <asm/byteorder.h>
#include <asm/sni.h>

#define mkaddr(bus, devfn, where)					\
do {									\
	if (bus->number == 0)						\
		return -1;						\
	*(volatile u32 *)PCIMT_CONFIG_ADDRESS =				\
		 ((bus->number    & 0xff) << 0x10) |			\
	         ((devfn & 0xff) << 0x08) |				\
	         (where  & 0xfc);					\
} while(0)

/*
 * We can't address 8 and 16 bit words directly.  Instead we have to
 * read/write a 32bit word and mask/modify the data we actually want.
 */
static int pcimt_read(struct pci_bus *bus, unsigned int devfn, int where,
		      int size, u32 * val)
{
	u32 res;

	switch (size) {
	case 1:
		mkaddr(bus, devfn, where);
		res = *(volatile u32 *) PCIMT_CONFIG_DATA;
		res = (le32_to_cpu(res) >> ((where & 3) << 3)) & 0xff;
		*val = (u8) res;
		break;
	case 2:
		if (where & 1)
			return PCIBIOS_BAD_REGISTER_NUMBER;
		mkaddr(bus, devfn, where);
		res = *(volatile u32 *) PCIMT_CONFIG_DATA;
		res = (le32_to_cpu(res) >> ((where & 3) << 3)) & 0xffff;
		*val = (u16) res;
		break;
	case 4:
		if (where & 3)
			return PCIBIOS_BAD_REGISTER_NUMBER;
		mkaddr(bus, devfn, where);
		res = *(volatile u32 *) PCIMT_CONFIG_DATA;
		res = le32_to_cpu(res);
		*val = res;
		break;
	}

	return PCIBIOS_SUCCESSFUL;
}

static int pcimt_write(struct pci_bus *bus, unsigned int devfn, int where,
		       int size, u32 val)
{
	switch (size) {
	case 1:
		mkaddr(bus, devfn, where);
		*(volatile u8 *) (PCIMT_CONFIG_DATA + (where & 3)) =
		    (u8) le32_to_cpu(val);
		break;
	case 2:
		if (where & 1)
			return PCIBIOS_BAD_REGISTER_NUMBER;
		mkaddr(bus, devfn, where);
		*(volatile u16 *) (PCIMT_CONFIG_DATA + (where & 3)) =
		    (u16) le32_to_cpu(val);
		break;
	case 4:
		if (where & 3)
			return PCIBIOS_BAD_REGISTER_NUMBER;
		mkaddr(bus, devfn, where);
		*(volatile u32 *) PCIMT_CONFIG_DATA = le32_to_cpu(val);
		break;
	}

	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops sni_pci_ops = {
	.read = pcimt_read,
	.write = pcimt_write,
};
