/*
 * yeeloong specific STR/Standby support
 *
 *  Copyright (C) 2009 Lemote Inc. & Insititute of Computing Technology
 *  Author: Wu Zhangjin <wuzj@lemote.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/suspend.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/i8042.h>

#include <asm/i8259.h>
#include <asm/delay.h>
#include <asm/mipsregs.h>

#include <loongson.h>

#define I8042_KBD_IRQ		1
#define I8042_CTR_KBDINT	0x01
#define I8042_CTR_KBDDIS	0x10

static unsigned char i8042_ctr;

static int i8042_enable_kbd_port(void)
{
	if (i8042_command(&i8042_ctr, I8042_CMD_CTL_RCTR)) {
		printk(KERN_ERR "i8042.c: Can't read CTR while enabling i8042 kbd port.\n");
		return -EIO;
	}

	i8042_ctr &= ~I8042_CTR_KBDDIS;
	i8042_ctr |= I8042_CTR_KBDINT;

	if (i8042_command(&i8042_ctr, I8042_CMD_CTL_WCTR)) {
		i8042_ctr &= ~I8042_CTR_KBDINT;
		i8042_ctr |= I8042_CTR_KBDDIS;
		printk(KERN_ERR "i8042.c: Failed to enable KBD port.\n");
		return -EIO;
	}

	return 0;
}

/* i8042 is connnectted to i8259A */
static void mach_setup_wakeup_events(void)
{
	int irq_mask;

	printk(KERN_INFO "setup wakeup interrupts\n");

	/* open the keyboard irq in i8259A */
	outb((0xff & ~(1 << I8042_KBD_IRQ)), PIC_MASTER_IMR);
	irq_mask = inb(PIC_MASTER_IMR);

	/* enable keyboard port */
	i8042_enable_kbd_port();
}
void setup_wakeup_events(void)
__attribute__((alias("mach_setup_wakeup_events")));

static int mach_wakeup_loongson(void)
{
	int irq;

	/* query the interrupt number */
	irq = mach_i8259_irq();
	if (irq < 0)
		return 0;

	printk(KERN_INFO "irq = %d\n", irq);

	if (irq == I8042_KBD_IRQ)
		return 1;
	return 0;
}
int wakeup_loongson(void)
__attribute__((alias("mach_wakeup_loongson")));
