/*
 *
 * BRIEF MODULE DESCRIPTION
 *	Galileo EV96100 board specific pci support.
 *
 * Copyright 2000 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *         	ppopov@mvista.com or source@mvista.com
 *
 * This file was derived from Carsten Langgaard's 
 * arch/mips/mips-boards/generic/pci.c
 *
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/config.h>

#ifdef CONFIG_PCI

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <asm//gt64120.h>
#include <asm/galileo-boards/ev96100.h>

#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1

#undef DEBUG

static int
mips_pcibios_config_access(unsigned char access_type, struct pci_dev *dev,
                           unsigned char where, u32 *data)
{
	unsigned char bus = dev->bus->number;
	unsigned char dev_fn = dev->devfn;
        u32 intr;


	if ((bus == 0) && (dev_fn >= PCI_DEVFN(31,0))) {
            return -1; /* Because of a bug in the galileo (for slot 31). */
        }

	/* Clear cause register bits */
	GT_WRITE(GT_INTRCAUSE_OFS, ~(GT_INTRCAUSE_MASABORT0_BIT | 
	                             GT_INTRCAUSE_TARABORT0_BIT));

	/* Setup address */
	GT_WRITE(GT_PCI0_CFGADDR_OFS,  
		 (bus         << GT_PCI0_CFGADDR_BUSNUM_SHF)   |
		 (dev_fn      << GT_PCI0_CFGADDR_FUNCTNUM_SHF) |
		 ((where / 4) << GT_PCI0_CFGADDR_REGNUM_SHF)   |
		 GT_PCI0_CFGADDR_CONFIGEN_BIT);


	if (access_type == PCI_ACCESS_WRITE) {
            if (dev_fn != 0) {
                *data = le32_to_cpu(*data);
            }
            GT_WRITE(GT_PCI0_CFGDATA_OFS, *data);
	} 
        else {
            GT_READ(GT_PCI0_CFGDATA_OFS, *data);
            if (dev_fn != 0) {
                *data = le32_to_cpu(*data);
            }
	}

	/* Check for master or target abort */
	GT_READ(GT_INTRCAUSE_OFS, intr);

	if (intr & (GT_INTRCAUSE_MASABORT0_BIT | GT_INTRCAUSE_TARABORT0_BIT))
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
static int
mips_pcibios_read_config_byte (struct pci_dev *dev, int where, u8 *val)
{
	u32 data = 0;

	if (mips_pcibios_config_access(PCI_ACCESS_READ, dev, where, &data))
		return -1;

	*val = (data >> ((where & 3) << 3)) & 0xff;
#if 0
        printk("cfg read byte: bus %d dev_fn %x where %x: val %x\n", 
                dev->bus->number, dev->devfn, where, *val);
#endif

	return PCIBIOS_SUCCESSFUL;
}


static int
mips_pcibios_read_config_word (struct pci_dev *dev, int where, u16 *val)
{
	u32 data = 0;

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (mips_pcibios_config_access(PCI_ACCESS_READ, dev, where, &data))
	       return -1;

	*val = (data >> ((where & 3) << 3)) & 0xffff;
#if 0
        printk("cfg read word: bus %d dev_fn %x where %x: val %x\n", 
                dev->bus->number, dev->devfn, where, *val);
#endif

	return PCIBIOS_SUCCESSFUL;
}

static int
mips_pcibios_read_config_dword (struct pci_dev *dev, int where, u32 *val)
{
	u32 data = 0;

	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;
	
	if (mips_pcibios_config_access(PCI_ACCESS_READ, dev, where, &data))
		return -1;

	*val = data;
#if 0
        printk("cfg read dword: bus %d dev_fn %x where %x: val %x\n", 
                dev->bus->number, dev->devfn, where, *val);
#endif

	return PCIBIOS_SUCCESSFUL;
}


static int
mips_pcibios_write_config_byte (struct pci_dev *dev, int where, u8 val)
{
	u32 data = 0;
       
	if (mips_pcibios_config_access(PCI_ACCESS_READ, dev, where, &data))
		return -1;

	data = (data & ~(0xff << ((where & 3) << 3))) |
	       (val << ((where & 3) << 3));

	if (mips_pcibios_config_access(PCI_ACCESS_WRITE, dev, where, &data))
		return -1;

	return PCIBIOS_SUCCESSFUL;
}

static int
mips_pcibios_write_config_word (struct pci_dev *dev, int where, u16 val)
{
        u32 data = 0;

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;
       
        if (mips_pcibios_config_access(PCI_ACCESS_READ, dev, where, &data))
	       return -1;

	data = (data & ~(0xffff << ((where & 3) << 3))) | 
	       (val << ((where & 3) << 3));

	if (mips_pcibios_config_access(PCI_ACCESS_WRITE, dev, where, &data))
	       return -1;


	return PCIBIOS_SUCCESSFUL;
}

static int
mips_pcibios_write_config_dword(struct pci_dev *dev, int where, u32 val)
{
	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	if (mips_pcibios_config_access(PCI_ACCESS_WRITE, dev, where, &val))
	       return -1;

	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops mips_pci_ops = {
	mips_pcibios_read_config_byte,
        mips_pcibios_read_config_word,
	mips_pcibios_read_config_dword,
	mips_pcibios_write_config_byte,
	mips_pcibios_write_config_word,
	mips_pcibios_write_config_dword
};

void __init pcibios_init(void)
{

	printk("PCI: Probing PCI hardware on host bus 0.\n");
	pci_scan_bus(0, &mips_pci_ops, NULL);

	/* 
	 * Due to a bug in the Galileo system controller, we need to setup 
	 * the PCI BAR for the Galileo internal registers.
	 * This should be done in the bios/bootprom and will be fixed in
	 * a later revision of YAMON (the MIPS boards boot prom).
	 */
	GT_WRITE(GT_PCI0_CFGADDR_OFS, cpu_to_le32(
		 (0 << GT_PCI0_CFGADDR_BUSNUM_SHF)   |  /* Local bus */
		 (0 << GT_PCI0_CFGADDR_DEVNUM_SHF)   |  /* GT64120 device */
		 (0 << GT_PCI0_CFGADDR_FUNCTNUM_SHF) |  /* Function 0 */
		 ((0x20/4) << GT_PCI0_CFGADDR_REGNUM_SHF) |  /* BAR 4 */
		 GT_PCI0_CFGADDR_CONFIGEN_BIT ));

	/* Perform the write */
	GT_WRITE( GT_PCI0_CFGDATA_OFS, cpu_to_le32(PHYSADDR(MIPS_GT_BASE))); 

}

int __init
pcibios_enable_device(struct pci_dev *dev)
{
	u16 cmd, old_cmd;
	int idx;
	struct resource *r;

	pci_read_config_word(dev, PCI_COMMAND, &cmd);
	old_cmd = cmd;
	for(idx=0; idx<6; idx++) {
		r = &dev->resource[idx];
		if (!r->start && r->end) {
			printk(KERN_ERR "PCI: Device %s not available because of resource collisions\n", dev->slot_name);
			return -EINVAL;
		}
		if (r->flags & IORESOURCE_IO)
			cmd |= PCI_COMMAND_IO;
		if (r->flags & IORESOURCE_MEM)
			cmd |= PCI_COMMAND_MEMORY;
	}
	if (dev->resource[PCI_ROM_RESOURCE].start)
		cmd |= PCI_COMMAND_MEMORY;
	if (cmd != old_cmd) {
		printk("PCI: Enabling device %s (%04x -> %04x)\n", dev->slot_name, old_cmd, cmd);
		pci_write_config_word(dev, PCI_COMMAND, cmd);
	}
	return 0;
}

void __init
pcibios_align_resource(void *data, struct resource *res, unsigned long size)
{
    printk("pcibios_align_resource\n");
#if 0 /* from ppc */
	struct pci_dev *dev = data;

	if (res->flags & IORESOURCE_IO) {
		unsigned long start = res->start;

		if (size > 0x100) {
			printk(KERN_ERR "PCI: I/O Region %s/%d too large"
			       " (%ld bytes)\n", dev->slot_name,
			       dev->resource - res, size);
		}

		if (start & 0x300) {
			start = (start + 0x3ff) & ~0x3ff;
			res->start = start;
		}
	}
#endif
}

char * __init
pcibios_setup(char *str)
{
	/* Nothing to do for now.  */

	return str;
}

void __init
pcibios_update_resource(struct pci_dev *dev, struct resource *root,
                        struct resource *res, int resource)
{
	unsigned long where, size;
	u32 reg;

	where = PCI_BASE_ADDRESS_0 + (resource * 4);
	size = res->end - res->start;
	pci_read_config_dword(dev, where, &reg);
	reg = (reg & size) | (((u32)(res->start - root->start)) & ~size);
	pci_write_config_dword(dev, where, reg);
}

/*
 *  Called after each bus is probed, but before its children
 *  are examined.
 */
void __init pcibios_fixup_bus(struct pci_bus *b)
{
//	pci_read_bridge_bases(b);
}

void __init ev96100_int_line_fixup(struct pci_dev *dev)     
{
	unsigned int slot;
	unsigned char irq;
	unsigned long vendor;

	/*
	** EV96100 interrupt routing for pci bus 0
	** NOTE: this are my experimental findings, since I do not
	** have Galileo's latest PLD equations.
	**
	** The functions in irq.c assume the following irq numbering:
	** irq 2: CPU cause register bit IP2
	** irq 3: CPU cause register bit IP3
	** irq 4: CPU cause register bit IP4
	** irq 5: CPU cause register bit IP5
	** irq 6: CPU cause register bit IP6
	** irq 7: CPU cause register bit IP7
	**
	*/

#ifdef DEBUG
	printk("ev96100_int_line_fixup bus %d\n", dev->bus->number);
#endif
	if (dev->bus->number != 0)
		return;

	slot = PCI_SLOT(dev->devfn);
	pci_read_config_word(dev, PCI_SUBSYSTEM_VENDOR_ID, &vendor);

#ifdef DEBUG
	printk("devfn %x, slot %d vendor %x\n", dev->devfn, slot, vendor);
#endif

	/* fixup irq line based on slot # */

	if (slot == 8) {
	    dev->irq = 5;
	    pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq);
	}
	else if (slot == 9) {
	    dev->irq = 2;
	    pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq);
	}
}

struct pci_fixup pcibios_fixups[] = {
	{ PCI_FIXUP_HEADER, PCI_ANY_ID, PCI_ANY_ID, ev96100_int_line_fixup },
	{ 0 }
};
#endif /* CONFIG_PCI */
