/*
 *  arch/mips/philips/nino/setup.c
 *
 *  Copyright (C) 2001 Steven J. Hill (sjhill@realitydiluted.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Interrupt and exception initialization for Philips Nino
 */
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/reboot.h>
#include <asm/time.h>
#include <asm/tx3912.h>

static void nino_machine_restart(char *command)
{
	static void (*back_to_prom)(void) = (void (*)(void)) 0xbfc00000;

	/* Reboot */
	back_to_prom();
}

static void nino_machine_halt(void)
{
	printk("Nino halted.\n");
	while(1);
}

static void nino_machine_power_off(void)
{
	printk("Nino halted. Please turn off power.\n");
	while(1);
}

static void __init nino_board_init()
{
	/* Nothing for now */
}

static __init void nino_time_init(void)
{
	unsigned int scratch = 0;

	RTCperiodTimer = PER_TIMER_COUNT;
	RTCtimerControl = TIM_ENPERTIMER;
	IntEnable5 |= INT5_PERIODICINT;

	scratch = inl(TX3912_CLK_CTRL_BASE);
	scratch |= TX3912_CLK_CTRL_ENTIMERCLK;
	outl(scratch, TX3912_CLK_CTRL_BASE);
}

static __init void nino_timer_setup(struct irqaction *irq)
{
	irq->dev_id = (void *) irq;
	setup_irq(0, irq);
}

void __init nino_setup(void)
{
	extern void nino_irq_setup(void);
	extern void nino_wait(void);

	irq_setup = nino_irq_setup;
	mips_io_port_base = KSEG1ADDR(0x10c00000);

	_machine_restart = nino_machine_restart;
	_machine_halt = nino_machine_halt;
	_machine_power_off = nino_machine_power_off;

	board_time_init = nino_time_init;
	board_timer_setup = nino_timer_setup;

	cpu_wait = nino_wait;

	nino_board_init();
}
