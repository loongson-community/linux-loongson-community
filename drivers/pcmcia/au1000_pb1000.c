/*
 * Alchemy Semi PB1000 board specific pcmcia routines.
 *
 * Copyright 2001 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *         	ppopov@mvista.com or source@mvista.com
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * 
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/config.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/tqueue.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/types.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/ss.h>
#include <pcmcia/bulkmem.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/bus_ops.h>
#include "cs_internal.h"

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>

#include <asm/au1000.h>
#include <asm/au1000_pcmcia.h>
#include <asm/pb1000.h>


extern struct pcmcia_x_table x_table;

static int pb1000_pcmcia_init(struct pcmcia_init *init)
{
	u32 pcr;
	pcr = PCR_SLOT_0_RST | PCR_SLOT_1_RST;
	//writel(readl(PIN_FUNCTION) & ~(1<<8), PIN_FUNCTION); /* pin15 is gpio */
	writel(0, PIN_FUNCTION);	/* pin15 is gpio */
	writel(readl(TSTATE_STATE_SET) | (1 << 15), TSTATE_STATE_SET);	/* tristate gpio15 */
	au_sync();

	writel(0x8000, AU1000_MDR);	/* clear pcmcia interrupt */
	writel(0x4000, AU1000_MDR);	/* enable pcmcia interrupt */
	au_sync();

	/* There's two sockets, but only the first one, 0, is used and tested */
	return 1;
}

static int pb1000_pcmcia_shutdown(void)
{
	u16 pcr;
	pcr = 0;
	writew(pcr, AU1000_PCR);
	mdelay(20);
	return 0;
}

static int
pb1000_pcmcia_socket_state(unsigned sock, struct pcmcia_state *state)
{
	u16 levels, pcr;
	unsigned char vs;

	levels = readw(AU1000_ACR1);
	pcr = readw(AU1000_PCR);

	state->ready = 0;
	state->vs_Xv = 0;
	state->vs_3v = 0;
	state->detect = 0;

	/* 
	 * CD1/2 are active low; so are the VSS pins; Ready is active high
	 */
	if (sock == 0) {
		if ((levels & ACR1_SLOT_0_READY))
			state->ready = 1;
		if (!(levels & (ACR1_SLOT_0_CD1 | ACR1_SLOT_0_CD2))) {
			state->detect = 1;
			vs = (levels >> 4) & 0x3;
			switch (vs) {
			case 0:
			case 1:
				DEBUG(1, "%d: vs_3v\n", sock);
				state->vs_3v = 1;
				break;
			case 2:
				state->vs_Xv = 1;
				DEBUG(1, "%d: vs_Xv\n", sock);
				break;
			case 3:
			default:
				break;
			}
		}
	} else if (sock == 1) {
		if ((levels & ACR1_SLOT_1_READY))
			state->ready = 1;
		if (!(levels & (ACR1_SLOT_1_CD1 | ACR1_SLOT_1_CD2))) {
			state->detect = 1;
			vs = (levels >> 12) & 0x3;
			switch (vs) {
			case 0:
			case 1:
				state->vs_3v = 1;
				DEBUG(1, "%d: vs_3v\n", sock);
				break;
			case 2:
				state->vs_Xv = 1;
				DEBUG(1, "%d: vs_Xv\n", sock);
				break;
			case 3:
			default:
				break;
			}
		}
	} else {
		printk(KERN_ERR "pb1000 socket_state bad sock %d\n", sock);
	}

	state->bvd1 = 1;
	state->bvd2 = 1;
	state->wrprot = 0;
	return 1;
}


static int pb1000_pcmcia_get_irq_info(struct pcmcia_irq_info *info)
{

	if (info->sock > PCMCIA_MAX_SOCK)
		return -1;

	if (info->sock == 0)
		info->irq = AU1000_GPIO_15;
	else
		info->irq = -1;

	return 0;
}


static int
pb1000_pcmcia_configure_socket(const struct pcmcia_configure *configure)
{
	u16 pcr;

	if (configure->sock > PCMCIA_MAX_SOCK)
		return -1;

	pcr = readw(AU1000_PCR);

	if (configure->sock == 0)
		pcr &= ~(PCR_SLOT_0_VCC0 | PCR_SLOT_0_VCC1);
	else
		pcr &= ~(PCR_SLOT_1_VCC0 | PCR_SLOT_1_VCC1);

	pcr &= ~PCR_SLOT_0_RST;
	writew(pcr, AU1000_PCR);
	mdelay(20);

	switch (configure->vcc) {
	case 0:		/* Vcc 0 */
		switch (configure->vpp) {
		case 0:
			pcr |=
			    SET_VCC_VPP(VCC_HIZ, VPP_GND, configure->sock);
			DEBUG(3, "Vcc 0V Vpp 0V, pcr %x\n", pcr);
			break;
		case 12:
			pcr |=
			    SET_VCC_VPP(VCC_HIZ, VPP_12V, configure->sock);
			DEBUG(3, "Vcc 0V Vpp 12V, pcr %x\n", pcr);
			break;
		case 50:
			pcr |=
			    SET_VCC_VPP(VCC_HIZ, VPP_5V, configure->sock);
			DEBUG(3, "Vcc 0V Vpp 5V, pcr %x\n", pcr);
			break;
		case 33:
		default:
			pcr |=
			    SET_VCC_VPP(VCC_HIZ, VPP_HIZ, configure->sock);
			printk(KERN_ERR "%s: bad Vcc/Vpp combo (%d:%d)\n",
			       __FUNCTION__, configure->vcc,
			       configure->vpp);
			break;
		}
		break;
	case 50:		/* Vcc 5V */
		switch (configure->vpp) {
		case 0:
			pcr |=
			    SET_VCC_VPP(VCC_5V, VPP_GND, configure->sock);
			DEBUG(3, "Vcc 5V Vpp 0V, pcr %x\n", pcr);
			break;
		case 50:
			pcr |=
			    SET_VCC_VPP(VCC_5V, VPP_5V, configure->sock);
			DEBUG(3, "Vcc 5V Vpp 5V, pcr %x\n", pcr);
			break;
		case 12:
			pcr |=
			    SET_VCC_VPP(VCC_5V, VPP_12V, configure->sock);
			DEBUG(3, "Vcc 5V Vpp 12V, pcr %x\n", pcr);
			break;
		case 33:
		default:
			pcr |=
			    SET_VCC_VPP(VCC_HIZ, VPP_HIZ, configure->sock);
			printk(KERN_ERR "%s: bad Vcc/Vpp combo (%d:%d)\n",
			       __FUNCTION__, configure->vcc,
			       configure->vpp);
			break;
		}
		break;
	case 33:		/* Vcc 3.3V */
		switch (configure->vpp) {
		case 0:
			pcr |=
			    SET_VCC_VPP(VCC_3V, VPP_GND, configure->sock);
			DEBUG(3, "Vcc 3V Vpp 0V, pcr %x\n", pcr);
			break;
		case 50:
			pcr |=
			    SET_VCC_VPP(VCC_3V, VPP_5V, configure->sock);
			DEBUG(3, "Vcc 3V Vpp 5V, pcr %x\n", pcr);
			break;
		case 12:
			pcr |=
			    SET_VCC_VPP(VCC_3V, VPP_12V, configure->sock);
			DEBUG(3, "Vcc 3V Vpp 12V, pcr %x\n", pcr);
			break;
		case 33:
		default:
			pcr |=
			    SET_VCC_VPP(VCC_HIZ, VPP_HIZ, configure->sock);
			printk(KERN_ERR "%s: bad Vcc/Vpp combo (%d:%d)\n",
			       __FUNCTION__, configure->vcc,
			       configure->vpp);
			break;
		}
		break;
	default:		/* what's this ? */
		pcr |= SET_VCC_VPP(VCC_HIZ, VPP_HIZ, configure->sock);
		printk(KERN_ERR "%s: bad Vcc %d\n", __FUNCTION__,
		       configure->vcc);
		break;
	}

	writew(pcr, AU1000_PCR);
	mdelay(400);

	pcr &= ~PCR_SLOT_0_RST;
	if (configure->reset) {
		pcr |= PCR_SLOT_0_RST;
	}
	writew(pcr, AU1000_PCR);
	mdelay(200);
	return 0;
}

struct pcmcia_low_level pb1000_pcmcia_ops = {
	pb1000_pcmcia_init,
	pb1000_pcmcia_shutdown,
	pb1000_pcmcia_socket_state,
	pb1000_pcmcia_get_irq_info,
	pb1000_pcmcia_configure_socket
};
