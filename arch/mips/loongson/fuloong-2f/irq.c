/*
 * Copyright (C) 2007 Lemote Inc. & Insititute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/interrupt.h>

#include <asm/irq_cpu.h>
#include <asm/i8259.h>

#include <loongson.h>
#include <machine.h>

#define LOONGSON_TIMER_IRQ	(MIPS_CPU_IRQ_BASE + 7)	/* cpu timer */
#define LOONGSON_PERFCNT_IRQ	(MIPS_CPU_IRQ_BASE + 6) /* cpu perf counter */
#define LOONGSON_NORTH_BRIDGE_IRQ	(MIPS_CPU_IRQ_BASE + 6)	/* bonito */
#define LOONGSON_UART_IRQ	(MIPS_CPU_IRQ_BASE + 3)	/* cpu serial port */
#define LOONGSON_SOUTH_BRIDGE_IRQ	(MIPS_CPU_IRQ_BASE + 2)	/* i8259 */

#define LOONGSON_INT_BIT_INT0		(1 << 11)
#define LOONGSON_INT_BIT_INT1		(1 << 12)

static int mach_i8259_irq(void)
{
	int irq, isr, imr;

	irq = -1;

	if ((LOONGSON_INTISR & LOONGSON_INTEN) & LOONGSON_INT_BIT_INT0) {
		imr = inb(0x21) | (inb(0xa1) << 8);
		isr = inb(0x20) | (inb(0xa0) << 8);
		isr &= ~0x4;	/* irq2 for cascade */
		isr &= ~imr;
		irq = ffs(isr) - 1;
	}

	return irq;
}

static void i8259_irqdispatch(void)
{
	int irq;

	irq = mach_i8259_irq();
	if (irq >= 0)
		do_IRQ(irq);
	else
		spurious_interrupt();
}

void mach_irq_dispatch(unsigned int pending)
{
	if (pending & CAUSEF_IP7)
		do_IRQ(LOONGSON_TIMER_IRQ);
	else if (pending & CAUSEF_IP6) {	/* North Bridge, Perf counter */
		do_IRQ(LOONGSON2_PERFCNT_IRQ);
		bonito_irqdispatch();
	} else if (pending & CAUSEF_IP3)	/* CPU UART */
		do_IRQ(LOONGSON_UART_IRQ);
	else if (pending & CAUSEF_IP2)	/* South Bridge */
		i8259_irqdispatch();
	else
		spurious_interrupt();
}

void __init set_irq_trigger_mode(void)
{
	/* setup cs5536 as high level trigger */
	LOONGSON_INTPOL = LOONGSON_INT_BIT_INT0 | LOONGSON_INT_BIT_INT1;
	LOONGSON_INTEDGE &= ~(LOONGSON_INT_BIT_INT0 | LOONGSON_INT_BIT_INT1);
}

void __init mach_init_irq(void)
{
	/* init all controller
	 *   0-15         ------> i8259 interrupt
	 *   16-23        ------> mips cpu interrupt
	 *   32-63        ------> bonito irq
	 */

	/* Sets the first-level interrupt dispatcher. */
	mips_cpu_irq_init();
	init_i8259_irqs();
	bonito_irq_init();

	/* setup north bridge irq (bonito) */
	setup_irq(LOONGSON_NORTH_BRIDGE_IRQ, &ip6_irqaction);
	/* setup source bridge irq (i8259) */
	setup_irq(LOONGSON_SOUTH_BRIDGE_IRQ, &cascade_irqaction);
}
