#include <linux/init.h>
#include <linux/pci.h>

void __init pcibios_fixup_irqs(void)
{
	/* This needs to be written ...  */
}

struct pci_fixup pcibios_fixups[] __initdata = {
	{ 0 }
};
