/*
 *
 * BRIEF MODULE DESCRIPTION
 *	EV96100 Board specific pci fixups.
 *
 * Copyright 2001 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *         	ppopov@mvista.com or source@mvista.com
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

#include <asm/gt64120.h>
#include <asm/galileo-boards/ev96100.h>

void __init pcibios_fixup_resources(struct pci_dev *dev)
{
}

void __init pcibios_fixup(void)
{
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

void __init pcibios_fixup_irqs(void)
{
	struct pci_dev *dev;
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


	pci_for_each_dev(dev) {
		if (dev->bus->number != 0)
			return;

		slot = PCI_SLOT(dev->devfn);
		pci_read_config_word(dev, PCI_SUBSYSTEM_VENDOR_ID, &vendor);

//#ifdef DEBUG
		printk("devfn %x, slot %d vendor %x\n", dev->devfn, slot, vendor);
//#endif

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
}
unsigned int pcibios_assign_all_busses(void)
{
	return 0;
}
#endif
