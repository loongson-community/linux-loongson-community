/*
 * FILE NAME
 *	arch/mips/vr41xx/victor-mpc30x/pci_fixup.c
 *
 * BRIEF MODULE DESCRIPTION
 *	The Victor MP-C303/304 specific PCI fixups.
 *
 * Copyright 2002 Yoichi Yuasa
 *                yuasa@hh.iij4u.or.jp
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/pci.h>

#include <asm/vr41xx/mpc30x.h>

void __init pcibios_fixup_resources(struct pci_dev *dev)
{
}

void __init pcibios_fixup(void)
{
}

void __init pcibios_fixup_irqs(void)
{
	struct pci_dev *dev;
	u8 slot, func;

	pci_for_each_dev(dev) {
		slot = PCI_SLOT(dev->devfn);
		func = PCI_FUNC(dev->devfn);
		dev->irq = 0;

		switch (slot) {
#ifdef CONFIG_VRC4173
		case 12:	/* NEC VRC4173 CARDU1 */
			dev->irq = PCMCIA1_IRQ;
			break;
		case 13:	/* NEC VRC4173 CARDU2 */
			dev->irq = PCMCIA2_IRQ;
			break;
#endif
		case 29:	/* mediaQ MQ-200 */
			dev->irq = MQ200_IRQ;
			break;
#ifdef CONFIG_VRC4173
		case 30:
			switch (func) {
			case 0:	/* NEC VRC4173 */
				dev->irq = VRC4173_CASCADE_IRQ;
				break;
			case 1:	/* NEC VRC4173 AC97U */
				dev->irq = AC97_IRQ;
				break;
			case 2:	/* NEC VRC4173 USBU */
				dev->irq = USB_IRQ;
				break;
			}
			break;
#endif
		}

		pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq);
	}
}

unsigned int pcibios_assign_all_busses(void)
{
	return 0;
}
