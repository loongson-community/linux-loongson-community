#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/pci.h>

void __init pcibios_fixup_irqs(void)
{
	panic("pcibios_fixup_irqs: implement me");
}

struct pci_fixup pcibios_fixups[] = {
	{0}
};
