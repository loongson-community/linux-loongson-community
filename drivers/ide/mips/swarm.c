/*
 * Copyright (C) 2001, 2002, 2003 Broadcom Corporation
 * Copyright (C) 2004 MontaVista Software Inc.
 *	Author:	Manish Lachwani, mlachwani@mvista.com
 * Copyright (C) 2004  MIPS Technologies, Inc.  All rights reserved.
 *	Author: Maciej W. Rozycki <macro@mips.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 *  Derived loosely from ide-pmac.c, so:  
 *  Copyright (C) 1998 Paul Mackerras.
 *  Copyright (C) 1995-1998 Mark Lord
 */

/*
 * Boards with SiByte processors so far have supported IDE devices via
 * the Generic Bus, PCI bus, and built-in PCMCIA interface.  In all
 * cases, byte-swapping must be avoided for these devices (whereas
 * other PCI devices, for example, will require swapping).  Any
 * SiByte-targetted kernel including IDE support will include this
 * file.  Probing of a Generic Bus for an IDE device is controlled by
 * the definition of "SIBYTE_HAVE_IDE", which is provided by
 * <asm/sibyte/board.h> for Broadcom boards.
 */

#include <linux/ide.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include <asm/io.h>

#include <asm/sibyte/board.h>
#include <asm/sibyte/sb1250_genbus.h>
#include <asm/sibyte/sb1250_regs.h>

#define DRV_NAME "ide-swarm"

static struct resource swarm_ide_resource = {
	.name	= "SWARM GenBus IDE",
	.flags	= IORESOURCE_MEM,
};

/*
 * swarm_ide_probe - if the board header indicates the existence of
 * Generic Bus IDE, allocate a HWIF for it.
 */
void __init swarm_ide_probe(void)
{
	ide_hwif_t *hwif;
	u8 __iomem *base;
	phys_t offset, size;
	int i;

	if (!SIBYTE_HAVE_IDE)
		return;

	/* Find an empty slot.  */
	for (i = 0; i < MAX_HWIFS; i++) 
		if (!ide_hwifs[i].io_ports[IDE_DATA_OFFSET])
			break;
	if (i >= MAX_HWIFS) {
		printk(KERN_ERR DRV_NAME ": no free slot for interface\n");
		return;
	}
	hwif = ide_hwifs + i;

	base = ioremap(A_IO_EXT_BASE, 0x800);
	offset = bus_readq(base + R_IO_EXT_REG(R_IO_EXT_START_ADDR, IDE_CS));
	size = bus_readq(base + R_IO_EXT_REG(R_IO_EXT_MULT_SIZE, IDE_CS));
	iounmap(base);
	
	offset = G_IO_START_ADDR(offset) << S_IO_ADDRBASE;
	size = (G_IO_MULT_SIZE(size) + 1) << S_IO_REGSIZE;
	if (offset < A_PHYS_GENBUS || offset >= A_PHYS_GENBUS_END) {
		printk(KERN_INFO DRV_NAME
		       ": IDE interface at GenBus disabled\n");
		return;
	}

	printk(KERN_INFO DRV_NAME ": IDE interface at GenBus slot %i\n",
	       IDE_CS);

	swarm_ide_resource.start = offset;
	swarm_ide_resource.end = offset + size - 1;
	if (request_resource(&iomem_resource, &swarm_ide_resource)) {
		printk(KERN_ERR DRV_NAME
		       ": can't request I/O memory resource\n");
		return;
	}

	base = ioremap(offset, size);

	/* Setup MMIO ops.  */
	default_hwif_mmiops(hwif);
	/* Prevent resource map manipulation.  */
	hwif->mmio = 2;
	hwif->noprobe = 0;

	for (i = IDE_DATA_OFFSET; i <= IDE_STATUS_OFFSET; i++)
		hwif->hw.io_ports[i] =
				(unsigned long)(base + ((0x1f0 + i) << 5));
	hwif->hw.io_ports[IDE_CONTROL_OFFSET] =
				(unsigned long)(base + (0x3f6 << 5));
	hwif->hw.irq = K_INT_GB_IDE;

	memcpy(hwif->io_ports, hwif->hw.io_ports, sizeof(hwif->io_ports));
	hwif->irq = hwif->hw.irq;
}
