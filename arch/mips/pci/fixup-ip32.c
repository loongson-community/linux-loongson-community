#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <asm/pci_channel.h>
#include <asm/ip32/mace.h>
#include <asm/ip32/crime.h>
#include <asm/ip32/ip32_ints.h>
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
	if ((_bus)->number > 0 || PCI_SLOT (_devfn) < 1		\
	    || PCI_SLOT (_devfn) > 3)			        \
		return PCIBIOS_DEVICE_NOT_FOUND;		\
} while (0)


/*
 * Given a PCI slot number (a la PCI_SLOT(...)) and the interrupt pin of
 * the device (1-4 => A-D), tell what irq to use.  Note that we don't
 * in theory have slots 4 and 5, and we never normally use the shared
 * irqs.  I suppose a device without a pin A will thank us for doing it
 * right if there exists such a broken piece of crap.
 */
static int __devinit macepci_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	chkslot(dev->bus, dev->devfn);
	if (pin == 0)
		pin = 1;
	switch (slot) {
	case 1:
		return MACEPCI_SCSI0_IRQ;
	case 2:
		return MACEPCI_SCSI1_IRQ;
	case 3:
		switch (pin) {
		case 2:
			return MACEPCI_SHARED0_IRQ;
		case 3:
			return MACEPCI_SHARED1_IRQ;
		case 4:
			return MACEPCI_SHARED2_IRQ;
		case 1:
		default:
			return MACEPCI_SLOT0_IRQ;
		}
	case 4:
		switch (pin) {
		case 2:
			return MACEPCI_SHARED2_IRQ;
		case 3:
			return MACEPCI_SHARED0_IRQ;
		case 4:
			return MACEPCI_SHARED1_IRQ;
		case 1:
		default:
			return MACEPCI_SLOT1_IRQ;
		}
		return MACEPCI_SLOT1_IRQ;
	case 5:
		switch (pin) {
		case 2:
			return MACEPCI_SHARED1_IRQ;
		case 3:
			return MACEPCI_SHARED2_IRQ;
		case 4:
			return MACEPCI_SHARED0_IRQ;
		case 1:
		default:
			return MACEPCI_SLOT2_IRQ;
		}
	default:
		return 0;
	}
}

void __init pcibios_fixup_irqs(void)
{
	pci_fixup_irqs(common_swizzle, macepci_map_irq);
}

struct pci_fixup pcibios_fixups[] = {
	{0}
};
