/*
 * Lasat 200 specific PCI support.
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>

#include <asm/lasat/lasat200.h>
#include <asm/nile4.h>

#undef DEBUG_PCI

#define LO(reg) (reg / 4)
#define HI(reg) (reg / 4 + 1)

volatile unsigned long * const vrc_pciregs = (void *)Vrc5074_BASE;


#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1


static int lasat_pcibios_config_access(unsigned char access_type,
				       struct pci_dev *dev,
				       unsigned char reg,
				       u32 *data)
{
	unsigned char bus = dev->bus->number;
	unsigned char dev_fn = dev->devfn;
	u32 adr, mask, err;

	if ((bus == 0) && (PCI_SLOT(dev_fn) > 8))
		/* The addressing scheme chosen leaves room for just
		 * 8 devices on the first bus (besides the PCI
		 * controller itself) */
		return -1;

	if ((bus == 0) && (dev_fn == PCI_DEVFN(0,0))) {
		/* Access controller registers directly */
		if (access_type == PCI_ACCESS_WRITE) {
			vrc_pciregs[(0x200+reg) >> 2] = *data;
		} else {
			*data = vrc_pciregs[(0x200+reg) >> 2];
		}
	        return 0;
	}

#if 0
	/* 
	 * This is a nasty hack. Because the old kernel
	 * added the devices in reverse order, Masquerade and
	 * safepipe depend on the fact that eth0 is "LAN 1"
	 * and eth1 is "LAN 2". So we play a game of make-believe.
	 */
	if (bus == 0) {
		if (PCI_SLOT(dev_fn) == 0x04) {
			dev_fn = PCI_DEVFN(0x05, PCI_FUNC(dev_fn));
		} else if (PCI_SLOT(dev_fn) == 0x05) {
			dev_fn = PCI_DEVFN(0x04, PCI_FUNC(dev_fn));
		}
	}
#endif

	/* Temporarily map PCI Window 1 to config space */
	mask = vrc_pciregs[LO(NILE4_PCIINIT1)];
	vrc_pciregs[LO(NILE4_PCIINIT1)] = 0x0000001a | (bus ? 0x200 : 0);

	/* Clear PCI Error register. This also clears the Error Type
	 * bits in the Control register */
	vrc_pciregs[LO(NILE4_PCIERR)] = 0;
	vrc_pciregs[HI(NILE4_PCIERR)] = 0;

	/* Setup address */
	if (bus == 0)
	        adr = KSEG1ADDR(PCI_WINDOW1) + ((1 << (PCI_SLOT(dev_fn) + 15)) | (PCI_FUNC(dev_fn) << 8) | (reg & ~3));
	else
	        adr = KSEG1ADDR(PCI_WINDOW1) | (bus << 16) | (dev_fn << 8) | (reg & ~3);

#ifdef DEBUG_PCI
	printk("PCI config %s: adr %x", access_type == PCI_ACCESS_WRITE ? "write" : "read", adr);
#endif

	if (access_type == PCI_ACCESS_WRITE) {
	        *(u32 *)adr = *data;
	} else {
	        *data = *(u32 *)adr;
	}

#ifdef DEBUG_PCI
	printk(" value = %x\n", *data);
#endif

	/* Check for master or target abort */
	err = (vrc_pciregs[HI(NILE4_PCICTRL)] >> 5) & 0x7;

	/* Restore PCI Window 1 */
	vrc_pciregs[LO(NILE4_PCIINIT1)] = mask;

	if (err)
	{
		/* Error occured */
#ifdef DEBUG_PCI
	        printk("\terror %x at adr %x\n", err, vrc_pciregs[LO(PCIERR)]);
#endif
		return -1;
	}

	return 0;
}


/*
 * We can't address 8 and 16 bit words directly.  Instead we have to
 * read/write a 32bit word and mask/modify the data we actually want.
 */
static int lasat_pcibios_read_config_byte(struct pci_dev *dev, int reg, u8 *val)
{
        u32 data=0, flags;

	save_and_cli(flags);

	if (lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data)) {
		restore_flags(flags);
		return -1;
	}

	*val = (data >> ((reg & 3) << 3)) & 0xff;

	restore_flags(flags);
	return PCIBIOS_SUCCESSFUL;
}


static int lasat_pcibios_read_config_word(struct pci_dev *dev, int reg, u16 *val)
{
        u32 data=0, flags;

	if (reg & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	save_and_cli(flags);

	if (lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data)) {
		restore_flags(flags);
		return -1;
	}

	*val = (data >> ((reg & 3) << 3)) & 0xffff;

	restore_flags(flags);
	return PCIBIOS_SUCCESSFUL;
}

static int lasat_pcibios_read_config_dword(struct pci_dev *dev, int reg, u32 *val)
{
        u32 data=0, flags;

	if (reg & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	save_and_cli(flags);

	if (lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data)) {
		restore_flags(flags);
		return -1;
	}

	*val = data;

	restore_flags(flags);
	return PCIBIOS_SUCCESSFUL;
}


static int lasat_pcibios_write_config_byte(struct pci_dev *dev, int reg, u8 val)
{
        u32 data=0, flags, err;

	save_and_cli(flags);

        err = lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data);
        if (err)
		goto out;

	data = (data & ~(0xff << ((reg & 3) << 3))) |
		(val << ((reg & 3) << 3));

	err = lasat_pcibios_config_access(PCI_ACCESS_WRITE, dev, reg, &data);
out:
	restore_flags(flags);

	if (err)
		return -1;
	else
		return PCIBIOS_SUCCESSFUL;
}

static int lasat_pcibios_write_config_word(struct pci_dev *dev, int reg, u16 val)
{
	u32 data=0, flags, err;

	if (reg & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;
       
	save_and_cli(flags);

        err = lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data);
        if (err)
	       goto out;

	data = (data & ~(0xffff << ((reg & 3) << 3))) | 
	       (val << ((reg & 3) << 3));

	err = lasat_pcibios_config_access(PCI_ACCESS_WRITE, dev, reg, &data);
out:
	restore_flags(flags);

	if (err)
		return -1;
	else
		return PCIBIOS_SUCCESSFUL;
}

static int lasat_pcibios_write_config_dword(struct pci_dev *dev, int reg, u32 val)
{
        u32 flags, err;

	if (reg & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	save_and_cli(flags);

	err = lasat_pcibios_config_access(PCI_ACCESS_WRITE, dev, reg, &val);
	restore_flags(flags);

	if (err)
		return -1;
	else
		return PCIBIOS_SUCCESSFUL;
}

struct pci_ops lasat_pci_ops_200 = {
	lasat_pcibios_read_config_byte,
	lasat_pcibios_read_config_word,
	lasat_pcibios_read_config_dword,
	lasat_pcibios_write_config_byte,
	lasat_pcibios_write_config_word,
	lasat_pcibios_write_config_dword
};
