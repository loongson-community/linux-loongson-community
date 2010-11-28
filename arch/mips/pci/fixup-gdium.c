/*
 * Copyright (C) 2010 yajin <yajin@vm-kernel.org>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/init.h>
#include <linux/pci.h>

#include <loongson.h>
/*
 * http://www.pcidatabase.com
 * GDIUM has different PCI mapping
 *  slot 13 (0x1814/0x0301) -> RaLink rt2561 Wireless-G PCI
 *  slog 14 (0x126f/0x0501) -> sm501
 *  slot 15 (0x1033/0x0035) -> NEC Dual OHCI controllers
 *                             plus Single EHCI controller
 *  slot 16 (0x10ec/0x8139) -> Realtek 8139c
 *  slot 17 (0x1033/0x00e0) -> NEC USB 2.0 Host Controller
 */

#undef INT_IRQA
#undef INT_IRQB
#undef INT_IRQC
#undef INT_IRQD
#define INT_IRQA 36
#define INT_IRQB 37
#define INT_IRQC 38
#define INT_IRQD 39

int __init pcibios_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	int irq = 0;

	switch (slot) {
	case 13:
		irq = INT_IRQC + ((pin - 1) & 3);
		break;
	case 14:
		irq = INT_IRQA;
		break;
	case 15:
#if CONFIG_GDIUM_VERSION > 2
		irq = INT_IRQB;
#else
		irq = INT_IRQA + ((pin - 1) & 3);
#endif
		break;
	case 16:
		irq = INT_IRQD;
		break;
#if CONFIG_GDIUM_VERSION > 2
	case 17:
		irq = INT_IRQC;
		break;
#endif
	default:
		pr_info(" strange pci slot number %d on gdium.\n", slot);
		break;
	}
	return irq;
}

/* Do platform specific device initialization at pci_enable_device() time */
int pcibios_plat_dev_init(struct pci_dev *dev)
{
	return 0;
}

/* Fixups for the USB host controllers */
static void __init gdium_usb_host_fixup(struct pci_dev *dev)
{
	unsigned int val;
	pci_read_config_dword(dev, 0xe0, &val);
#if CONFIG_GDIUM_VERSION > 2
	pci_write_config_dword(dev, 0xe0, (val & ~3) | 0x3);
#else
	pci_write_config_dword(dev, 0xe0, (val & ~7) | 0x5);
	pci_write_config_dword(dev, 0xe4, 1<<5);
#endif
}
DECLARE_PCI_FIXUP_HEADER(PCI_VENDOR_ID_NEC, PCI_DEVICE_ID_NEC_USB,
				gdium_usb_host_fixup);
#if CONFIG_GDIUM_VERSION > 2
DECLARE_PCI_FIXUP_HEADER(PCI_VENDOR_ID_NEC, PCI_DEVICE_ID_CT_65550,
				gdium_usb_host_fixup);
#endif
