static void ddb5477_fixup(struct pci_dev *dev)
{
	u8 old;

	printk(KERN_NOTICE "Enabling ALI M1533/35 PS2 keyboard/mouse.\n");
	pci_read_config_byte(dev, 0x41, &old);
	pci_write_config_byte(dev, 0x41, old | 0xd0);
}

/*
 * Fixup baseboard AMD chip so that tx does not underflow.
 *      bcr_18 |= 0x0800
 * This sets NOUFLO bit which makes tx not start until whole pkt
 * is fetched to the chip.
 */
#define PCNET32_WIO_RDP		0x10
#define PCNET32_WIO_RAP		0x12
#define PCNET32_WIO_RESET	0x14
#define PCNET32_WIO_BDP		0x16

static void ddb5477_amd_lance_fixup(struct pci_dev *dev)
{
	unsigned long ioaddr;
	u16 temp;

	ioaddr = pci_resource_start(dev, 0);

	inw(ioaddr + PCNET32_WIO_RESET);	/* reset chip */
                                                                                
	/* bcr_18 |= 0x0800 */
	outw(18, ioaddr + PCNET32_WIO_RAP);
	temp = inw(ioaddr + PCNET32_WIO_BDP);
	temp |= 0x0800;
	outw(18, ioaddr + PCNET32_WIO_RAP);
	outw(temp, ioaddr + PCNET32_WIO_BDP);
}

struct pci_fixup pcibios_fixups[] __initdata = {
	{ PCI_FIXUP_FINAL, PCI_VENDOR_ID_AL, PCI_DEVICE_ID_AL_M1533,
	  ddb5477_fixup },
	{ PCI_FIXUP_FINAL, PCI_VENDOR_ID_AL, PCI_DEVICE_ID_AL_M1535,
	  ddb5477_fixup },
	{ PCI_FIXUP_FINAL, PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_LANCE,
	  ddb5477_amd_lance_fixup },
	{0}
};
