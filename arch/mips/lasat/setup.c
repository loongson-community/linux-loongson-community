/*
 * setup.c
 *
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999 MIPS Technologies, Inc.  All rights reserved.
 *
 * Thomas Horsten <thh@lasat.com>
 * Copyright (C) 2000 LASAT Networks A/S.
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
 * Lasat specific setup.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/ide.h>

#include <linux/interrupt.h>
#include <asm/time.h>

#include <asm/cpu.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/lasat/lasat.h>

#include <asm/lasat/lasat_mtd.h>

#if CONFIG_PICVUE
#include <linux/notifier.h>
#endif

#include "ds1603.h"

int lasat_command_line = 0;
void lasatint_init(void);

#ifdef CONFIG_BLK_DEV_IDE
extern struct ide_ops std_ide_ops;
extern struct ide_ops *ide_ops;
#endif

extern char arcs_cmdline[CL_SIZE];

extern void lasat_reboot_setup(void);
extern void pcisetup(void);
extern void edhac_init(void *, void *, void *);
extern void addrflt_init(void);

void __init bus_error_init(void) { /* nothing */ }

#ifdef CONFIG_PICVUE
#include "picvue.h"

static int lasat_panic_event(struct notifier_block *this,
			     unsigned long event, void *ptr)
{
	unsigned char *string = ptr;
	if (string == NULL)
		string = "Kernel Panic";
	pvc_write_string(string, 0, 0);
	if (strlen(string) > PVC_VISIBLE_CHARS)
		pvc_write_string(&string[PVC_VISIBLE_CHARS], 0, 1);
	return NOTIFY_DONE;
}

static struct notifier_block lasat_panic_block = {
	lasat_panic_event,
	NULL,
	INT_MAX /* try to do it first */
};
#endif

static int lasat_ide_default_irq(ide_ioreg_t base) {
	return 0;
}

static ide_ioreg_t lasat_ide_default_io_base(int index) {
	return 0;
}

void lasat_time_init()
{
	mips_counter_frequency = lasat_get_cpu_hz() / 2;
}

void lasat_timer_setup(struct irqaction *irq)
{

	write_32bit_cp0_register(CP0_COMPARE, 
		read_32bit_cp0_register(CP0_COUNT) + 
		mips_counter_frequency / HZ);
	change_cp0_status(ST0_IM, IE_IRQ0 | IE_IRQ5);
}

#define MIPS_CPU_TIMER_IRQ 7
asmlinkage void lasat_timer_interrupt(struct pt_regs *regs)
{
	ll_timer_interrupt(MIPS_CPU_TIMER_IRQ, regs);
}

void __init lasat_setup(void)
{
#ifdef CONFIG_PICVUE
	/* Set up panic notifier */
	notifier_chain_register(&panic_notifier_list, &lasat_panic_block);
#endif

#ifdef CONFIG_BLK_DEV_IDE
	ide_ops = &std_ide_ops;
	ide_ops->ide_default_irq = &lasat_ide_default_irq;
	ide_ops->ide_default_io_base = &lasat_ide_default_io_base;
#endif

	lasat_reboot_setup();

	board_time_init = lasat_time_init;
	board_timer_setup = lasat_timer_setup;
#ifdef CONFIG_DS1603
	rtc_get_time = ds1603_read;
	rtc_set_time = ds1603_set;
#endif

	/* Switch from prom exception handler to normal mode */
	change_cp0_status(ST0_BEV,0);

	prom_printf("platform_setup done\n");
}

#ifdef CONFIG_LASAT_SERVICE
/*
 * Called by init() (just before starting user space init program)
 */
static int __init lasat_init(void)
{
	/* This is where we call service mode */
	lasat_service();
	for (;;) {}	/* We should never get here */

	return 0;
}

__initcall(lasat_init);
#endif

unsigned long lasat_flash_partition_start(int partno)
{
	unsigned long dst;

	switch (partno) {
	case LASAT_MTD_BOOTLOADER:
		dst = lasat_board_info.li_flash_service_base;
		break;
	case LASAT_MTD_SERVICE:
		dst = lasat_board_info.li_flash_service_base + BOOTLOADER_SIZE;
		break;
	case LASAT_MTD_NORMAL:
		dst = lasat_board_info.li_flash_normal_base;
		break;
	case LASAT_MTD_FS:
		dst = lasat_board_info.li_flash_fs_base;
		break;
	case LASAT_MTD_CONFIG:
		dst = lasat_board_info.li_flash_cfg_base;
		break;
	default:
		dst = 0;
		break;
	}

	return dst;
}

unsigned long lasat_flash_partition_size(int partno)
{
	unsigned long size;

	switch (partno) {
	case LASAT_MTD_BOOTLOADER:
		size = BOOTLOADER_SIZE;
		break;
	case LASAT_MTD_SERVICE:
		size = lasat_board_info.li_flash_service_size - BOOTLOADER_SIZE;
		break;
	case LASAT_MTD_NORMAL:
		size = lasat_board_info.li_flash_normal_size;
		break;
	case LASAT_MTD_FS:
		size = lasat_board_info.li_flash_fs_size;
		break;
	case LASAT_MTD_CONFIG:
		size = lasat_board_info.li_flash_cfg_size;
		break;
	default:
		size = 0;
		break;
	}

	return size;
}
