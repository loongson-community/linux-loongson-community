/*
 * Copyright (C) 2001 Broadcom Corporation
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

#include <linux/config.h>
#include <linux/kernel.h>
#include <asm/sibyte/sb1250_regs.h>
#include <asm/sibyte/sb1250_int.h>
#include <asm/mipsregs.h>
#include "cfe_xiocb.h"
#include "cfe_api.h"



extern void asmlinkage smp_bootstrap(void);
/* Boot all other cpus in the system, initialize them, and
   bring them into the boot fn */
int prom_boot_secondary(int cpu, unsigned long sp, unsigned long gp)
{
	int retval;
	if ((retval = cfe_start_cpu(1, &smp_bootstrap, sp, gp, 0)) != 0) {
		printk("cfe_start_cpu returned %i\n" , retval);
		panic("secondary bootstrap failed");
	}
	return 1;
}


void prom_init_secondary(void)
{
	

	/* Set up kseg0 to be cachable coherent */
	clear_cp0_config(CONF_CM_CMASK);
	set_cp0_config(0x5);

	/* Enable interrupts for lines 0-4 */
	clear_cp0_status(0xe000);
	set_cp0_status(0x1f01);
}


/*
 * Set up state, return the total number of cpus in the system, including
 * the master
 */
int prom_setup_smp(void)
{
	/* Nothing to do here */
	return 2;
}

void prom_smp_finish(void)
{
	sb1250_smp_finish();
}
