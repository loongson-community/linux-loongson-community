/*
 * bios32.c - Fake PCI BIOS functions for RM200 C systems.  Chainsawed
 *            from the Alpha version.
 *
 * Written by Ralf Baechle (ralf@gnu.ai.mit.edu)
 *
 * For more information, please consult
 *
 * PCI BIOS Specification Revision
 * PCI Local Bus Specification
 * PCI System Design Guide
 *
 * PCI Special Interest Group
 * M/S HF3-15A
 * 5200 N.E. Elam Young Parkway
 * Hillsboro, Oregon 97124-6497
 * +1 (503) 696-2000
 * +1 (800) 433-5177
 *
 * Manuals are $25 each or $50 for all three, plus $7 shipping
 * within the United States, $35 abroad.
 */
#include <linux/config.h>

#include <linux/kernel.h>
#include <linux/bios32.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/malloc.h>
#include <linux/mm.h>

#include <asm/ptrace.h>
#include <asm/system.h>
#include <asm/io.h>

/*
 * This is a table of the onboard PCI devices of the RM200 C
 * onboard devices.
 */
static struct board {
	unsigned short vendor;
	unsigned short device_id;
	unsigned int memory;
	unsigned int io;
	unsigned char irq;
	unsigned char revision;
	} boards[] = {
	{
		PCI_VENDOR_ID_NCR,
		PCI_DEVICE_ID_NCR_53C810,
		0x1b000000,
		0x00000000,
		5,
		0x11
	},
	{
		PCI_VENDOR_ID_AMD,
		PCI_DEVICE_ID_AMD_LANCE,
		0x1b000100,
		0x17beffe0,
		6,
		0x16
	},
	{
		PCI_VENDOR_ID_CIRRUS,
		PCI_DEVICE_ID_CIRRUS_5434_8,
		0x18000000,
		0x00000000,
		4,
		0x8e
	},
	{ 0xffff, }
};

/*
 * Given the vendor and device ids, find the n'th instance of that device
 * in the system.  
 */
int pcibios_find_device (unsigned short vendor, unsigned short device_id,
			 unsigned short index, unsigned char *bus,
			 unsigned char *devfn)
{
	unsigned int curr = 0;
	struct board *p;

	for (p = pci_devices; p->vendor != 0xffff; p++) {
		if (p->vendor == vendor && p->device == device_id) {
			if (curr == index) {
				*devfn = p->devfn;
				*bus = 0;
				return PCIBIOS_SUCCESSFUL;
			}
			++curr;
		}
	}

	return PCIBIOS_DEVICE_NOT_FOUND;
}

/*
 * Given the class, find the n'th instance of that device
 * in the system.
 */
int pcibios_find_class (unsigned int class_code, unsigned short index,
			unsigned char *bus, unsigned char *devfn)
{
        unsigned int curr = 0;
	struct pci_dev *dev;

	for (dev = pci_devices; dev; dev = dev->next) {
		if (dev->class == class_code) {
			if (curr == index) {
				*devfn = dev->devfn;
				*bus = dev->bus->number;
				return PCIBIOS_SUCCESSFUL;
			}
			++curr;
		}
	}
	return PCIBIOS_DEVICE_NOT_FOUND;
}

int pcibios_present(void)
{
        return 1;
}

unsigned long pcibios_init(unsigned long mem_start,
			   unsigned long mem_end)
{
	printk("SNI RM200 C BIOS32 fake implementation\n");

	return mem_start;
}

const char *pcibios_strerror (int error)
{
        static char buf[80];

        switch (error) {
                case PCIBIOS_SUCCESSFUL:
                        return "SUCCESSFUL";

                case PCIBIOS_FUNC_NOT_SUPPORTED:
                        return "FUNC_NOT_SUPPORTED";

                case PCIBIOS_BAD_VENDOR_ID:
                        return "SUCCESSFUL";

                case PCIBIOS_DEVICE_NOT_FOUND:
                        return "DEVICE_NOT_FOUND";

                case PCIBIOS_BAD_REGISTER_NUMBER:
                        return "BAD_REGISTER_NUMBER";

                default:
                        sprintf (buf, "UNKNOWN RETURN 0x%x", error);
                        return buf;
        }
}

/*
 * BIOS32-style PCI interface:
 */

int pcibios_read_config_byte (unsigned char bus, unsigned char device_fn,
			      unsigned char where, unsigned char *value)
{
	unsigned long addr = LCA_CONF;
	unsigned long pci_addr;

	*value = 0xff;

	if (mk_conf_addr(bus, device_fn, where, &pci_addr) < 0) {
		return PCIBIOS_SUCCESSFUL;
	}
	addr |= (pci_addr << 5) + 0x00;
	*value = conf_read(addr) >> ((where & 3) * 8);
	return PCIBIOS_SUCCESSFUL;
}


int pcibios_read_config_word (unsigned char bus, unsigned char device_fn,
			      unsigned char where, unsigned short *value)
{
	unsigned long addr = LCA_CONF;
	unsigned long pci_addr;

	*value = 0xffff;

	if (where & 0x1) {
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	if (mk_conf_addr(bus, device_fn, where, &pci_addr)) {
		return PCIBIOS_SUCCESSFUL;
	}
	addr |= (pci_addr << 5) + 0x08;
	*value = conf_read(addr) >> ((where & 3) * 8);
	return PCIBIOS_SUCCESSFUL;
}


int pcibios_read_config_dword (unsigned char bus, unsigned char device_fn,
			       unsigned char where, unsigned int *value)
{
	unsigned long addr = LCA_CONF;
	unsigned long pci_addr;

	*value = 0xffffffff;
	if (where & 0x3) {
		return PCIBIOS_BAD_REGISTER_NUMBER;
	}
	if (mk_conf_addr(bus, device_fn, where, &pci_addr)) {
		return PCIBIOS_SUCCESSFUL;
	}
	addr |= (pci_addr << 5) + 0x18;
	*value = conf_read(addr);
	return PCIBIOS_SUCCESSFUL;
}


int pcibios_write_config_byte (unsigned char bus, unsigned char device_fn,
			       unsigned char where, unsigned char value)
{
	unsigned long addr = LCA_CONF;
	unsigned long pci_addr;

	if (mk_conf_addr(bus, device_fn, where, &pci_addr) < 0) {
		return PCIBIOS_SUCCESSFUL;
	}
	addr |= (pci_addr << 5) + 0x00;
	conf_write(addr, value << ((where & 3) * 8));
	return PCIBIOS_SUCCESSFUL;
}


int pcibios_write_config_word (unsigned char bus, unsigned char device_fn,
			       unsigned char where, unsigned short value)
{
	unsigned long addr = LCA_CONF;
	unsigned long pci_addr;

	if (mk_conf_addr(bus, device_fn, where, &pci_addr) < 0) {
		return PCIBIOS_SUCCESSFUL;
	}
	addr |= (pci_addr << 5) + 0x08;
	conf_write(addr, value << ((where & 3) * 8));
	return PCIBIOS_SUCCESSFUL;
}


int pcibios_write_config_dword (unsigned char bus, unsigned char device_fn,
				unsigned char where, unsigned int value)
{
	unsigned long addr = LCA_CONF;
	unsigned long pci_addr;

	if (mk_conf_addr(bus, device_fn, where, &pci_addr) < 0) {
		return PCIBIOS_SUCCESSFUL;
	}
	addr |= (pci_addr << 5) + 0x18;
	conf_write(addr, value << ((where & 3) * 8));
	return PCIBIOS_SUCCESSFUL;
}
