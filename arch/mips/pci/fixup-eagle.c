/*
 * FILE NAME
 *	arch/mips/vr41xx/nec-eagle/pci_fixup.c
 *
 * BRIEF MODULE DESCRIPTION
 *	The NEC Eagle/Hawk Board specific PCI fixups.
 *
 * Author: Yoichi Yuasa
 *         yyuasa@mvista.com or source@mvista.com
 *
 * Copyright 2001,2002 MontaVista Software Inc.
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 *  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/init.h>
#include <linux/pci.h>

#include <asm/vr41xx/eagle.h>
#include <asm/vr41xx/vrc4173.h>

/*
 * Shortcuts
 */
#define INTA	CP_INTA_IRQ
#define INTB	CP_INTB_IRQ
#define INTC	CP_INTC_IRQ
#define INTD	CP_INTD_IRQ
#define PCMCIA1	VRC4173_PCMCIA1_IRQ
#define PCMCIA2	VRC4173_PCMCIA2_IRQ
#define LAN	LANINTA_IRQ
#define SLOT	PCISLOT_IRQ

static char irq_tab_eagle[][5] __initdata = {
 [ 8] = { 0,    INTA, INTB, INTC, INTD },
 [ 9] = { 0,    INTD, INTA, INTB, INTC },
 [10] = { 0,    INTC, INTD, INTA, INTB },
 [12] = { 0, PCMCIA1,    0,    0,    0 },
 [13] = { 0, PCMCIA2,    0,    0,    0 },
 [28] = { 0,     LAN,    0,    0,    0 },
 [29] = { 0,    SLOT, INTB, INTC, INTD },
};

/*
 * This is a multifunction device.
 */
static char irq_func_tab[] __initdata = {
	VRC4173_CASCADE_IRQ,
	VRC4173_AC97_IRQ,
	VRC4173_USB_IRQ
};

int __init pcibios_map_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	if (slot == 30)
		return irq_func_tab[PCI_FUNC(dev->devfn)];

	return irq_tab_eagle[slot][pin];
}
