/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000, 2001 Keith M Wesolowski
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <asm/pci_channel.h>
#include <asm/ip32/mace.h>
#include <asm/ip32/crime.h>
#include <asm/ip32/ip32_ints.h>
#include <linux/delay.h>

#undef DEBUG_MACE_PCI

/*
 * Handle errors from the bridge.  This includes master and target aborts,
 * various command and address errors, and the interrupt test.  This gets
 * registered on the bridge error irq.  It's conceivable that some of these
 * conditions warrant a panic.  Anybody care to say which ones?
 */
static irqreturn_t macepci_error(int irq, void *dev, struct pt_regs *regs)
{
	u32 flags, error_addr;
	char space;

	flags = mace_read_32(MACEPCI_ERROR_FLAGS);
	error_addr = mace_read_32(MACEPCI_ERROR_ADDR);

	if (flags & MACEPCI_ERROR_MEMORY_ADDR)
		space = 'M';
	else if (flags & MACEPCI_ERROR_CONFIG_ADDR)
		space = 'C';
	else
		space = 'X';

	if (flags & MACEPCI_ERROR_MASTER_ABORT) {
		printk("MACEPCI: Master abort at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(MACEPCI_ERROR_FLAGS,
			      flags & ~MACEPCI_ERROR_MASTER_ABORT);
	}
	if (flags & MACEPCI_ERROR_TARGET_ABORT) {
		printk("MACEPCI: Target abort at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(MACEPCI_ERROR_FLAGS,
			      flags & ~MACEPCI_ERROR_TARGET_ABORT);
	}
	if (flags & MACEPCI_ERROR_DATA_PARITY_ERR) {
		printk("MACEPCI: Data parity error at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(MACEPCI_ERROR_FLAGS, flags
			      & ~MACEPCI_ERROR_DATA_PARITY_ERR);
	}
	if (flags & MACEPCI_ERROR_RETRY_ERR) {
		printk("MACEPCI: Retry error at 0x%08x (%c)\n", error_addr,
		       space);
		mace_write_32(MACEPCI_ERROR_FLAGS, flags
			      & ~MACEPCI_ERROR_RETRY_ERR);
	}
	if (flags & MACEPCI_ERROR_ILLEGAL_CMD) {
		printk("MACEPCI: Illegal command at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(MACEPCI_ERROR_FLAGS,
			      flags & ~MACEPCI_ERROR_ILLEGAL_CMD);
	}
	if (flags & MACEPCI_ERROR_SYSTEM_ERR) {
		printk("MACEPCI: System error at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(MACEPCI_ERROR_FLAGS, flags
			      & ~MACEPCI_ERROR_SYSTEM_ERR);
	}
	if (flags & MACEPCI_ERROR_PARITY_ERR) {
		printk("MACEPCI: Parity error at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(MACEPCI_ERROR_FLAGS,
			      flags & ~MACEPCI_ERROR_PARITY_ERR);
	}
	if (flags & MACEPCI_ERROR_OVERRUN) {
		printk("MACEPCI: Overrun error at 0x%08x (%c)\n",
		       error_addr, space);
		mace_write_32(MACEPCI_ERROR_FLAGS, flags
			      & ~MACEPCI_ERROR_OVERRUN);
	}
	if (flags & MACEPCI_ERROR_SIG_TABORT) {
		printk("MACEPCI: Signaled target abort (clearing)\n");
		mace_write_32(MACEPCI_ERROR_FLAGS, flags
			      & ~MACEPCI_ERROR_SIG_TABORT);
	}
	if (flags & MACEPCI_ERROR_INTERRUPT_TEST) {
		printk("MACEPCI: Interrupt test triggered (clearing)\n");
		mace_write_32(MACEPCI_ERROR_FLAGS, flags
			      & ~MACEPCI_ERROR_INTERRUPT_TEST);
	}

	return IRQ_HANDLED;
}


extern struct pci_ops mace_pci_ops;
/*#ifdef CONFIG_MIPS64
static struct resource mace_pci_mem_resource = {
	.name	= "SGI O2 PCI MEM",
	.start	= UNCACHEDADDR(0x200000000UL),
	.end	= UNCACHEDADDR(0x2FFFFFFFFUL),
	.flags	= IORESOURCE_MEM,
};
static struct resource mace_pci_io_resource = {
	.name	= "SGI O2 PCI IO",
	.start	= 0x00000000UL,
	.end	= 0xffffffffUL,
	.flags	= IORESOURCE_IO,
};
#else
*/
static struct resource mace_pci_mem_resource = {
	.name	= "SGI O2 PCI MEM",
	.start	= MACEPCI_LOW_MEMORY,
	.end	= MACEPCI_LOW_MEMORY+32*1024*1024,
	.flags	= IORESOURCE_MEM,
};
static struct resource mace_pci_io_resource = {
	.name	= "SGI O2 PCI IO",
	.start	= 0x00000000,
	.end	= 0xFFFFFFFF,
	.flags	= IORESOURCE_IO,
};
//#endif
static struct pci_controller mace_pci_controller = {
	.pci_ops	= &mace_pci_ops,
	.mem_resource	= &mace_pci_mem_resource,
	.io_resource	= &mace_pci_io_resource
};


static void __init mace_init(void)
{
	/* Clear any outstanding errors and enable interrupts */
	mace_write_32(MACEPCI_ERROR_ADDR, 0);
	mace_write_32(MACEPCI_ERROR_FLAGS, 0);
	mace_write_32(MACEPCI_CONTROL, 0xff008500);
	crime_write_64(CRIME_HARD_INT, 0UL);
	crime_write_64(CRIME_SOFT_INT, 0UL);
	crime_write_64(CRIME_INT_STAT, 0x000000000000ff00UL);

	if (request_irq(MACE_PCI_BRIDGE_IRQ, macepci_error, 0,
			"MACE PCI error", NULL))
		panic("PCI bridge can't get interrupt; can't happen.");

}
//subsys_initcall(mace_init);

void __init ip32_pci_setup(void)
{
 	u32 rev = mace_read_32(MACEPCI_REV);

 	printk("MACE: PCI rev %d detected at %016lx\n", rev,
 	       (u64) MACE_BASE + MACE_PCI);

 	ioport_resource.start = 0;
	ioport_resource.end = mace_pci_io_resource.end;
	iomem_resource.start = mace_pci_mem_resource.start;
	iomem_resource.end = mace_pci_mem_resource.end;
	register_pci_controller(&mace_pci_controller);
}
