/*
 * Copyright (C) 2001, 2002, 2003 Broadcom Corporation
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

/*  Derived loosely from ide-pmac.c, so:
 *  
 *  Copyright (C) 1998 Paul Mackerras.
 *  Copyright (C) 1995-1998 Mark Lord
 *
 *  Copyright (C) 2004 MontaVista Software Inc.
 *  Author: Manish Lachwani, mlachwani@mvista.com
 */

/*
 * Boards with SiByte processors so far have supported IDE devices via
 * the Generic Bus, PCI bus, and built-in PCMCIA interface.  In all
 * cases, byte-swapping must be avoided for these devices (whereas
 * other PCI devices, for example, will require swapping).  Any
 * SiByte-targetted kernel including IDE support will include this
 * file.  Probing of a Generic Bus for an IDE device is controlled by
 * the definitions of "SIBYTE_HAVE_IDE" and "IDE_PHYS", which are
 * provided by <asm/sibyte/board.h> for Broadcom boards.
 */

#include <linux/kernel.h>
#include <linux/ide.h>
#include <asm/sibyte/board.h>

/*
 * Our non-swapping I/O operations.  
 */

static inline void sibyte_outb(u8 val, unsigned long port) {
	*(volatile u8 *)(mips_io_port_base + (port)) = val;
}

static inline void sibyte_outw(u16 val, unsigned long port) {
	*(volatile u16 *)(mips_io_port_base + (port)) = val;
}

static inline void sibyte_outl(u32 val, unsigned long port) {
	*(volatile u32 *)(mips_io_port_base + (port)) = val;
}

static inline unsigned char sibyte_inb(unsigned long port)
{
	return (*(volatile u8 *)(mips_io_port_base + (port)));
}

static inline unsigned short sibyte_inw(unsigned long port)
{
	return (*(volatile u16 *)(mips_io_port_base + (port)));
}

static inline unsigned int sibyte_inl(unsigned long port)
{
	return (*(volatile u32 *)(mips_io_port_base + (port)));
}


static inline void sibyte_outsb(unsigned long port, void *addr, unsigned int count)
{
	while (count--) {
		sibyte_outb(*(u8 *)addr, port);
		addr++;
	}
}

static inline void sibyte_insb(unsigned long port, void *addr, unsigned int count)
{
	while (count--) {
		*(u8 *)addr = sibyte_inb(port);
		addr++;
	}
}

static inline void sibyte_outsw(unsigned long port, void *addr, unsigned int count)
{
	while (count--) {
		sibyte_outw(*(u16 *)addr, port);
		addr += 2;
	}
}

static inline void sibyte_insw(unsigned long port, void *addr, unsigned int count)
{
	while (count--) {
		*(u16 *)addr = sibyte_inw(port);
		addr += 2;
	}
}

static inline void sibyte_outsl(unsigned long port, void *addr, unsigned int count)
{
	while (count--) {
		sibyte_outl(*(u32 *)addr, port);
		addr += 4;
	}
}

static inline void sibyte_insl(unsigned long port, void *addr, unsigned int count)
{
	while (count--) {
		*(u32 *)addr = sibyte_inl(port);
		addr += 4;
	}
}

static void sibyte_set_ideops(ide_hwif_t *hwif)
{
	hwif->INB = sibyte_inb;
	hwif->INW = sibyte_inw;
	hwif->INL = sibyte_inl;
	hwif->OUTB = sibyte_outb;
	hwif->OUTW = sibyte_outw;
	hwif->OUTL = sibyte_outl;
	hwif->INSW = sibyte_insw;
	hwif->INSL = sibyte_insl;
	hwif->OUTSW = sibyte_outsw;
	hwif->OUTSL = sibyte_outsl;
}

/*
 * swarm_ide_probe - if the board header indicates the existence of
 * Generic Bus IDE, allocate a HWIF for it.
 */
void __init swarm_ide_probe(void)
{
#if defined(SIBYTE_HAVE_IDE) && defined(IDE_PHYS)
	int i = 0;
	ide_hwif_t *sb_ide_hwif;

	for (i = 0; i < MAX_HWIFS; i++) 
		if (!ide_hwifs[i].io_ports[IDE_DATA_OFFSET]) {
			/* Find an empty slot */
			break;
		}

	/*
	 * Preadjust for mips_io_port_base since the I/O ops expect
	 * relative addresses
	 */
#define SIBYTE_IDE_REG(pcaddr) (IOADDR(IDE_PHYS) + ((pcaddr)<<5) - mips_io_port_base)

	sb_ide_hwif = &ide_hwifs[i];

	sb_ide_hwif->hw.io_ports[IDE_DATA_OFFSET]    = SIBYTE_IDE_REG(0x1f0);
	sb_ide_hwif->hw.io_ports[IDE_ERROR_OFFSET]   = SIBYTE_IDE_REG(0x1f1);
	sb_ide_hwif->hw.io_ports[IDE_NSECTOR_OFFSET] = SIBYTE_IDE_REG(0x1f2);
	sb_ide_hwif->hw.io_ports[IDE_SECTOR_OFFSET]  = SIBYTE_IDE_REG(0x1f3);
	sb_ide_hwif->hw.io_ports[IDE_LCYL_OFFSET]    = SIBYTE_IDE_REG(0x1f4);
	sb_ide_hwif->hw.io_ports[IDE_HCYL_OFFSET]    = SIBYTE_IDE_REG(0x1f5);
	sb_ide_hwif->hw.io_ports[IDE_SELECT_OFFSET]  = SIBYTE_IDE_REG(0x1f6);
	sb_ide_hwif->hw.io_ports[IDE_STATUS_OFFSET]  = SIBYTE_IDE_REG(0x1f7);
	sb_ide_hwif->hw.io_ports[IDE_CONTROL_OFFSET] = SIBYTE_IDE_REG(0x3f6);
	sb_ide_hwif->hw.io_ports[IDE_IRQ_OFFSET]     = SIBYTE_IDE_REG(0x3f7);

	sb_ide_hwif->hw.irq                          = K_INT_GB_IDE;
	sb_ide_hwif->irq			     = K_INT_GB_IDE;
	sb_ide_hwif->hw.ack_intr		     = NULL;
	sb_ide_hwif->noprobe			     = 0;

	memcpy(sb_ide_hwif->io_ports, sb_ide_hwif->hw.io_ports, sizeof(sb_ide_hwif->io_ports));
	
	printk("SiByte onboard IDE configured as device %d\n", i);

	/* Prevent resource map manipulation */
	sb_ide_hwif->mmio = 2;

	/* Reset the ideops */
	sibyte_set_ideops(sb_ide_hwif);
#endif
}

