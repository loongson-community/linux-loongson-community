/*
 * Lasat 100 specific PCI support.
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>

#include <asm/lasat/lasat100.h>
#include <asm/gt64120.h>


/*
 * Because of an error/peculiarity in the Galileo chip, we need to swap the 
 * bytes when running bigendian.
 */
#define GT_WRITE(ofs, data)  \
             *(volatile u32 *)(LASAT_GT_BASE+ofs) = cpu_to_le32(data)
#define GT_READ(ofs, data)   \
             data = le32_to_cpu(*(volatile u32 *)(LASAT_GT_BASE+ofs))


#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1

static int lasat_pcibios_config_access(unsigned char access_type,
				       struct pci_dev *dev,
				       unsigned char reg,
				       u32 *data)
{
	unsigned char bus = dev->bus->number;
	unsigned char dev_fn = dev->devfn;
        u32 intr;

	if ((bus == 0) && (dev_fn >= PCI_DEVFN(31,0)))
	        return -1; /* Because of a bug in the Galileo (for slot 31). */

#if 0
	/* 
	 * This is a nasty hack. Because the old kernel
	 * added the devices in reverse order, Masquerade and
	 * safepipe depend on the fact that eth0 is "LAN 1"
	 * and eth1 is "LAN 2". So we play a game of make-believe.
	 */
	if (bus == 0) {
		if (PCI_SLOT(dev_fn) == 0x12) {
			dev_fn = PCI_DEVFN(0x13, PCI_FUNC(dev_fn));
		} else if (PCI_SLOT(dev_fn) == 0x13) {
			dev_fn = PCI_DEVFN(0x12, PCI_FUNC(dev_fn));
		}
	}
#endif

	/* Clear cause register bits */
	GT_WRITE( GT_INTRCAUSE_OFS, ~(GT_INTRCAUSE_MASABORT0_BIT | 
				      GT_INTRCAUSE_TARABORT0_BIT) );

	/* Setup address */
	GT_WRITE( GT_PCI0_CFGADDR_OFS, 
		  (bus       << GT_PCI0_CFGADDR_BUSNUM_SHF) |
		  (dev_fn    << GT_PCI0_CFGADDR_FUNCTNUM_SHF) |
		  ((reg / 4) << GT_PCI0_CFGADDR_REGNUM_SHF)   |
		  GT_PCI0_CFGADDR_CONFIGEN_BIT );

	if (access_type == PCI_ACCESS_WRITE) {
	        GT_WRITE( GT_PCI0_CFGDATA_OFS, *data );
	} else {
	        GT_READ( GT_PCI0_CFGDATA_OFS, *data );
	}

	/* Check for master or target abort */
	GT_READ( GT_INTRCAUSE_OFS, intr );

	if( intr & (GT_INTRCAUSE_MASABORT0_BIT | GT_INTRCAUSE_TARABORT0_BIT) )
	{
	        /* Error occured */

	        /* Clear bits */
	        GT_WRITE( GT_INTRCAUSE_OFS, ~(GT_INTRCAUSE_MASABORT0_BIT | 
					      GT_INTRCAUSE_TARABORT0_BIT) );

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
        u32 data=0;

	if (lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data))
	       return -1;

	*val = (data >> ((reg & 3) << 3)) & 0xff;

	return PCIBIOS_SUCCESSFUL;
}


static int lasat_pcibios_read_config_word(struct pci_dev *dev, int reg, u16 *val)
{
        u32 data=0;

	if (reg & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data))
	       return -1;

	*val = (data >> ((reg & 3) << 3)) & 0xffff;

	return PCIBIOS_SUCCESSFUL;
}

static int lasat_pcibios_read_config_dword(struct pci_dev *dev, int reg, u32 *val)
{
        u32 data=0;

	if (reg & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;
	
	if (lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data))
	       return -1;

	*val = data;

	return PCIBIOS_SUCCESSFUL;
}


static int lasat_pcibios_write_config_byte(struct pci_dev *dev, int reg, u8 val)
{
        u32 data=0;
       
        if (lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data))
	       return -1;

	data = (data & ~(0xff << ((reg & 3) << 3))) |
	       (val << ((reg & 3) << 3));

	if (lasat_pcibios_config_access(PCI_ACCESS_WRITE, dev, reg, &data))
	       return -1;

	return PCIBIOS_SUCCESSFUL;
}

static int lasat_pcibios_write_config_word(struct pci_dev *dev, int reg, u16 val)
{
        u32 data=0;

	if (reg & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;
       
        if (lasat_pcibios_config_access(PCI_ACCESS_READ, dev, reg, &data))
	       return -1;

	data = (data & ~(0xffff << ((reg & 3) << 3))) | 
	       (val << ((reg & 3) << 3));

	if (lasat_pcibios_config_access(PCI_ACCESS_WRITE, dev, reg, &data))
	       return -1;


	return PCIBIOS_SUCCESSFUL;
}

static int lasat_pcibios_write_config_dword(struct pci_dev *dev, int reg, u32 val)
{
	if (reg & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (lasat_pcibios_config_access(PCI_ACCESS_WRITE, dev, reg, &val))
	       return -1;

	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops lasat_pci_ops_100 = {
	lasat_pcibios_read_config_byte,
	lasat_pcibios_read_config_word,
	lasat_pcibios_read_config_dword,
	lasat_pcibios_write_config_byte,
	lasat_pcibios_write_config_word,
	lasat_pcibios_write_config_dword
};
