/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * SNI specific PCI support for RM200/RM300.
 *
 * Copyright (C) 1997 - 2000, 2003 Ralf Baechle
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>

#include <asm/mipsregs.h>
#include <asm/pci_channel.h>
#include <asm/sni.h>

/*
 * Shortcuts ...
 */
#define SCSI	PCIMT_IRQ_SCSI
#define ETH	PCIMT_IRQ_ETHERNET
#define INTA	PCIMT_IRQ_INTA
#define INTB	PCIMT_IRQ_INTB
#define INTC	PCIMT_IRQ_INTC
#define INTD	PCIMT_IRQ_INTD

/*
 * Device 0: PCI EISA Bridge	(directly routed)
 * Device 1: NCR53c810 SCSI	(directly routed)
 * Device 2: PCnet32 Ethernet	(directly routed)
 * Device 3: Unsued
 * Device 4: Unused
 * Device 5: Slot 2
 * Device 6: Slot 3
 * Device 7: Slot 4	
 */
static char irq_tab[8][5] __initdata = {
	/*       INTA  INTB  INTC  INTD */
	{     0,    0,    0,    0,    0 },	/* EISA bridge */
	{  SCSI, SCSI, SCSI, SCSI, SCSI },	/* SCSI */
	{   ETH,  ETH,  ETH,  ETH,  ETH },	/* Ethernet */
	{     0,    0,    0,    0,    0 },	/* Unused */
	{     0,    0,    0,    0,    0 },	/* Unused */
	{     0, INTB, INTC, INTD, INTA },	/* Slot 2 */
	{     0, INTC, INTD, INTA, INTB },	/* Slot 3 */
	{     0, INTD, INTA, INTB, INTC },	/* Slot 4 */
};

static int __init sni_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	return irq_tab[slot][pin];
}

void __init pcibios_fixup_irqs(void)
{
	pci_fixup_irqs(common_swizzle, sni_map_irq);
}

struct pci_fixup pcibios_fixups[] = {
	{0}
};
