/*
 * setup.c
 *
 * BRIEF MODULE DESCRIPTION
 * Galileo Evaluation Boards - board dependent boot routines
 *
 * Copyright (C) 1996, 1997, 2001  Ralf Baechle
 * Copyright (C) 2000 RidgeRun, Inc.
 * Author: RidgeRun, Inc.
 *   glonnon@ridgerun.com, skranz@ridgerun.com, stevej@ridgerun.com
 *
 * Copyright 2001 MontaVista Software Inc.
 * Author: jsun@mvista.com or jsun@junsun.net
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
 *
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/timex.h>
#include <asm/bootinfo.h>
#include <asm/page.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/pci.h>
#include <asm/processor.h>
#include <asm/ptrace.h>
#include <asm/reboot.h>
#include <asm/mc146818rtc.h>
#include <linux/version.h>
#include <linux/bootmem.h>
#include <linux/blk.h>
#include <asm/gt64120/gt64120.h>

extern struct rtc_ops no_rtc_ops;
struct rtc_ops *rtc_ops;

/* These functions are used for rebooting or halting the machine*/
extern void momenco_ocelot_restart(char *command);
extern void momenco_ocelot_halt(void);
extern void momenco_ocelot_power_off(void);

char arcs_cmdline[COMMAND_LINE_SIZE]= { ""/*console=ttyS0,9600"*/ };

void (*board_time_init) (struct irqaction * irq);

extern void gt64120_time_init(void);
extern void momenco_ocelot_irq_setup(void);

void momenco_ocelot_setup(void)
{
	unsigned int i, j;

	irq_setup = momenco_ocelot_irq_setup;
	board_time_init = gt64120_time_init;

	mips_io_port_base = KSEG1;

	_machine_restart = momenco_ocelot_restart;
	_machine_halt = momenco_ocelot_halt;
	_machine_power_off = momenco_ocelot_power_off;

	/*
	 * initrd_start = (ulong)ocelot_initrd_start;
	 * initrd_end = (ulong)ocelot_initrd_start + (ulong)ocelot_initrd_size;
	 * initrd_below_start_ok = 1;
	 */
	rtc_ops = &no_rtc_ops;
}
