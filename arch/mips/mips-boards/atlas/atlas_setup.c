/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
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
 * Atlas specific setup.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/mc146818rtc.h>
#include <linux/ioport.h>
#include <linux/tty.h>
#include <linux/serial.h>
#include <linux/serial_core.h>

#include <asm/cpu.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/mips-boards/generic.h>
#include <asm/mips-boards/prom.h>
#include <asm/mips-boards/atlas.h>
#include <asm/mips-boards/atlasint.h>
#include <asm/gt64120.h>
#include <asm/time.h>
#include <asm/traps.h>

#ifdef CONFIG_KGDB
extern void rs_kgdb_hook(int);
extern void saa9730_kgdb_hook(void);
extern void breakpoint(void);
int remote_debug = 0;
#endif

extern struct rtc_ops atlas_rtc_ops;

extern void mips_reboot_setup(void);
extern void mips_time_init(void);
extern void mips_timer_setup(struct irqaction *irq);
extern unsigned long mips_rtc_get_time(void);

static void __init serial_init(void);

const char *get_system_type(void)
{
	return "MIPS Atlas";
}

void __init atlas_setup(void)
{
#ifdef CONFIG_KGDB
	int rs_putDebugChar(char);
	char rs_getDebugChar(void);
	int saa9730_putDebugChar(char);
	char saa9730_getDebugChar(void);
	extern int (*generic_putDebugChar)(char);
	extern char (*generic_getDebugChar)(void);
#endif
	char *argptr;

	ioport_resource.end = 0x7fffffff;

	serial_init ();

#ifdef CONFIG_KGDB
	argptr = prom_getcmdline();
	if ((argptr = strstr(argptr, "kgdb=ttyS")) != NULL) {
		int line;
		argptr += strlen("kgdb=ttyS");
		if (*argptr != '0' && *argptr != '1')
			printk("KGDB: Unknown serial line /dev/ttyS%c, "
			       "falling back to /dev/ttyS1\n", *argptr);
		line = *argptr == '0' ? 0 : 1;
		printk("KGDB: Using serial line /dev/ttyS%d for session\n",
		       line ? 1 : 0);

		if(line == 0) {
			rs_kgdb_hook(line);
			generic_putDebugChar = rs_putDebugChar;
			generic_getDebugChar = rs_getDebugChar;
		} else {
			saa9730_kgdb_hook();
			generic_putDebugChar = saa9730_putDebugChar;
			generic_getDebugChar = saa9730_getDebugChar;
		}

		prom_printf("KGDB: Using serial line /dev/ttyS%d for session, "
			    "please connect your debugger\n", line ? 1 : 0);

		remote_debug = 1;
		/* Breakpoints and stuff are in atlas_irq_setup() */
	}
#endif

	rtc_ops = &atlas_rtc_ops;
	rtc_get_time = mips_rtc_get_time;

	board_time_init = mips_time_init;
	board_timer_setup = mips_timer_setup;

	mips_reboot_setup();
}

static void __init serial_init(void)
{
#ifdef CONFIG_SERIAL_8250
	struct uart_port s;

	memset(&s, 0, sizeof(s));

#ifdef CONFIG_CPU_LITTLE_ENDIAN
	s.iobase = ATLAS_UART_REGS_BASE;
#else
	s.iobase = ATLAS_UART_REGS_BASE+3;
#endif
	s.irq = ATLASINT_UART;
	s.uartclk = ATLAS_BASE_BAUD * 16;
	s.flags = ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST | UPF_RESOURCES | ASYNC_AUTO_IRQ;
	s.iotype = 0;
	s.regshift = 3;

	if (early_serial_setup(&s) != 0) {
		printk(KERN_ERR "Serial setup failed!\n");
	}
#endif
}
