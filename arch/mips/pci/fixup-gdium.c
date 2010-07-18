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
 *
 */
int __init pcibios_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	switch (slot) {
	case 13:
		return LOONGSON_IRQ_BASE + ((pin - 1) & 3);
	case 14:
		return LOONGSON_IRQ_BASE + 4; /*36*/
	case 15:
		return LOONGSON_IRQ_BASE + 5; /*37*/
	case 16:
		return LOONGSON_IRQ_BASE + 7; /*39*/
	case 17:
		return LOONGSON_IRQ_BASE + 6; /*38*/
	default:
		pr_info(" strange pci slot number %d on gdium.\n", slot);
		return 0;
	}
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
	pci_write_config_dword(dev, 0xe0, (val & ~3) | 0x3);
}
DECLARE_PCI_FIXUP_HEADER(PCI_VENDOR_ID_NEC, PCI_DEVICE_ID_NEC_USB,
				gdium_usb_host_fixup);
DECLARE_PCI_FIXUP_HEADER(PCI_VENDOR_ID_NEC, PCI_DEVICE_ID_CT_65550,
				gdium_usb_host_fixup);
