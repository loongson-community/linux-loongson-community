/*
 * Copyright (C) 2000 RidgeRun, Inc.
 * Author: RidgeRun, Inc.
 *   glonnon@ridgerun.com, skranz@ridgerun.com, stevej@ridgerun.com
 *
 * Copyright 2001 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
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
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/timex.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/irq.h>
#include <asm/bitops.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mipsregs.h>
#include <asm/system.h>


#undef IRQ_DEBUG

#ifdef IRQ_DEBUG
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif


/* Function for careful CP0 interrupt mask access */
static inline void modify_cp0_intmask(unsigned clr_mask, unsigned set_mask)
{
	unsigned long status = read_32bit_cp0_register(CP0_STATUS);
	DBG(KERN_INFO "modify_cp0_intmask clr %x, set %x\n", clr_mask,
	    set_mask);
	DBG(KERN_INFO "modify_cp0_intmask status %x\n", status);
	status &= ~((clr_mask & 0xFF) << 8);
	status |= (set_mask & 0xFF) << 8;
	DBG(KERN_INFO "modify_cp0_intmask status %x\n", status);
	write_32bit_cp0_register(CP0_STATUS, status);
}

static inline void mask_irq(unsigned int irq_nr)
{
	modify_cp0_intmask(irq_nr, 0);
}

static inline void unmask_irq(unsigned int irq_nr)
{
	modify_cp0_intmask(0, irq_nr);
}

void disable_irq(unsigned int irq_nr)
{
	unsigned long flags;

	DBG(KERN_INFO "disable_irq, irq %d\n", irq_nr);
	save_and_cli(flags);
	/* we don't support higher interrupts, nor cascaded interrupts */
	if (irq_nr >= 8)
		panic("irq_nr is greater than 8");
	
	mask_irq(1 << irq_nr);
	restore_flags(flags);
}

void enable_irq(unsigned int irq_nr)
{
	unsigned long flags;

	save_and_cli(flags);
	
	if ( irq_nr >= 8 )
		panic("irq_nr is greater than 8");
	
	unmask_irq( 1 << irq_nr );
	restore_flags(flags);
}

/*
 * Ocelot irq setup -
 *
 * Initializes CPU interrupts
 *
 *
 * Inputs :
 *
 * Outpus :
 *
 */
void momenco_ocelot_irq_setup(void)
{
	extern asmlinkage void ocelot_handle_int(void);
	extern void gt64120_irq_init(void);

	DBG(KERN_INFO "rr: momenco_ocelot_irq_setup entry\n");

	gt64120_irq_init();

	/*
	 * Clear all of the interrupts while we change the able around a bit.
	 * int-handler is not on bootstrap
	 */
	clear_cp0_status(ST0_IM | ST0_BEV);

	/* Sets the first-level interrupt dispatcher. */
	set_except_vector(0, ocelot_handle_int);

	cli();

	/*
	 * Enable timer.  Other interrupts will be enabled as they are
	 * registered.
	 */
	// change_cp0_status(ST0_IM, IE_IRQ4);


#ifdef CONFIG_REMOTE_DEBUG
	{
		/*
		extern int DEBUG_CHANNEL;
		serial_init(DEBUG_CHANNEL);
		serial_set(DEBUG_CHANNEL, 115200);
		*/
		printk("start kgdb ...\n");
		set_debug_traps();
		breakpoint();	/* you may move this line to whereever you want :-) */
	}
#endif
}
