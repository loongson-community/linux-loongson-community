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
#include <linux/module.h>

#include <asm/i8259.h>
#include <asm/delay.h>
#include <asm/mipsregs.h>

#include <loongson.h>

#include <ec/kb3310b.h>

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

#define SCI_IRQ_NUM 0x0A	/* system control interface */
#define CASCADE_IRQ_NUM 2	/* cascade irq num of i8259A */

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

	/* there is a need to wakeup the cpu via sci interrupt with relative
	 * lid openning event
	 */
	outb(irq_mask & ~(1 << CASCADE_IRQ_NUM), PIC_MASTER_IMR);
	inb(PIC_MASTER_IMR);
	outb(0xff & ~(1 << (SCI_IRQ_NUM - 8)), PIC_SLAVE_IMR);
	inb(PIC_SLAVE_IMR);
}

void setup_wakeup_events(void)
__attribute__((alias("mach_setup_wakeup_events")));

static struct delayed_work lid_task;
static int initialized;
/* yeeloong_report_lid_status will be implemented in drivers/platform/loongson/yeeloong_laptop.c*/
sci_event_handler yeeloong_report_lid_status = NULL;
EXPORT_SYMBOL(yeeloong_report_lid_status);
static void yeeloong_lid_update_task(struct work_struct *work)
{
	if (yeeloong_report_lid_status)
		yeeloong_report_lid_status(BIT_LID_DETECT_ON);
}

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
	else if (irq == SCI_IRQ_NUM) {
		int ret, sci_event;
		/* query the event number */
		ret = ec_query_seq(CMD_GET_EVENT_NUM);
		if (ret < 0)
			return 0;
		sci_event = ec_get_event_num();
		if (sci_event < 0)
			return 0;
		if (sci_event == EVENT_LID) {
			int lid_status;
			/* check the LID status */
			lid_status = ec_read(REG_LID_DETECT);
			/* wakeup cpu when people open the LID */
			if (lid_status == BIT_LID_DETECT_ON) {
				/* send out this event */
				printk(KERN_INFO "send out lid open event in %s\n", __func__);

				/* if we call it directly here, the WARNING
				 * will happen at line 98 of
				 * kerenl/time/timekeeping.c (getnstimeofday)
				 * via "WARN_ON(timekeeping_suspended);" because,
				 * currently, we are in the suspend status
				 */
				if (initialized == 0) {
					/* init the rfkill work task */
					INIT_DELAYED_WORK(&lid_task, yeeloong_lid_update_task);
					initialized = 1;
				}
				schedule_delayed_work(&lid_task, 1);
				return 1;
			}
		}
	}

	return 0;
}
int wakeup_loongson(void)
__attribute__((alias("mach_wakeup_loongson")));
