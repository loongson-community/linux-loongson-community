/*
 * Common PCI stuff
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <asm/bootinfo.h>

#undef DEBUG_PCI
#ifdef DEBUG_PCI
#define Dprintk(fmt...) printk(fmt)
#else
#define Dprintk(fmt...)
#endif

extern struct pci_ops lasat_pci_ops_100;
extern struct pci_ops lasat_pci_ops_200;

static struct pci_ops *lasat_pci_ops;

char * __init pcibios_setup(char *str)
{
	return str;
}

static int __init pcibios_init(void)
{
	switch (mips_machtype) {
	case MACH_LASAT_100:
		lasat_pci_ops = &lasat_pci_ops_100;
		break;
	case MACH_LASAT_200:
		lasat_pci_ops = &lasat_pci_ops_200;
		break;
	default:
		panic("pcibios_init: mips_machtype incorrect");
	}

	pci_scan_bus(0, lasat_pci_ops, NULL);

	return 0;
}

subsys_initcall(pcibios_init);

void __init pcibios_fixup_bus(struct pci_bus *b)
{
	Dprintk("pcibios_fixup_bus()\n");
}

void pcibios_update_resource(struct pci_dev *dev, struct resource *root,
			     struct resource *res, int resource)
{
}

int __init pcibios_enable_device(struct pci_dev *dev, int mask)
{
	/* Not needed, since we enable all devices at startup.  */
	return 0;
}

void __init pcibios_align_resource(void *data, struct resource *res, 
		unsigned long size, unsigned long align)
{
}

unsigned __init int pcibios_assign_all_busses(void)
{
	return 1;
}

struct pci_fixup pcibios_fixups[] = {
	{ 0 }
};
