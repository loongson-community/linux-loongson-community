/*
 * Copyright 2001 MontaVista Software Inc.
 * Author: jsun@mvista.com or jsun@junsun.net
 *
 * arch/mips/ddb5xxx/common/irq.c
 *     Common irq code for DDB boards.  This really should belong
 *	arch/mips/kernel/irq.c.  Need to talk to Ralf.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <asm/irq.h>

void (*irq_setup)(void);

void __init arch_init_irq(void)
{
	/* invoke board-specific irq setup */
	irq_setup();
}
