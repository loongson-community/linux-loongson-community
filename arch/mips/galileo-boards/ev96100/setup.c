/*
 *
 * BRIEF MODULE DESCRIPTION
 *	Galileo EV96100 setup.
 *
 * Copyright 2000 MontaVista Software Inc.
 * Author: MontaVista Software, Inc.
 *         	ppopov@mvista.com or support@mvista.com
 *
 * This file was derived from Carsten Langgaard's 
 * arch/mips/mips-boards/atlas/atlas_setup.c.
 *
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
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
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/mc146818rtc.h>

#include <asm/cpu.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/galileo-boards/ev96100.h>
#include <asm/galileo-boards/ev96100int.h>
#include <asm/mipsregs.h>


#if defined(CONFIG_SERIAL_CONSOLE) || defined(CONFIG_PROM_CONSOLE)
extern void console_setup(char *, int *);
char serial_console[20];
#endif

#ifdef CONFIG_REMOTE_DEBUG
extern void rs_kgdb_hook(int);
extern void saa9730_kgdb_hook(void);
extern void breakpoint(void);
static int remote_debug = 0;
static int kgdb_on_pci = 0;
#endif

void (*board_time_init) (struct irqaction * irq);
extern void ev96100_time_init(struct irqaction *irq);

extern void mips_reboot_setup(void);
extern struct rtc_ops ev96100_rtc_ops;
extern struct resource ioport_resource;

static void __init ev96100_irq_setup(void)
{
	puts("ev96100_irq_setup");
	init_IRQ();

#ifdef CONFIG_REMOTE_DEBUG
	/* If local serial I/O used for debug port, enter kgdb at once */
	/* Otherwise, this will be done after the SAA9730 is up */
	if (remote_debug && !kgdb_on_pci) {
		set_debug_traps();
		breakpoint();
	}
#endif
}


void __init ev96100_setup(void)
{

	unsigned long mem_size, free_start, free_end, start_pfn,
	    bootmap_size;

#ifdef CONFIG_REMOTE_DEBUG
	int rs_putDebugChar(char);
	char rs_getDebugChar(void);
	int saa9730_putDebugChar(char);
	char saa9730_getDebugChar(void);
	extern int (*putDebugChar) (char);
	extern char (*getDebugChar) (void);
#endif
	char *argptr;

	irq_setup = ev96100_irq_setup;

	puts("ev96100_setup");
	puts("config reg:");
	put32(read_32bit_cp0_register(CP0_CONFIG));
	puts("");


#ifdef CONFIG_SERIAL_CONSOLE
	argptr = prom_getcmdline();
	if ((argptr = strstr(argptr, "console=ttyS0")) == NULL) {
		int i = 0;
		char *s = prom_getenv("modetty0");
		while (s[i] >= '0' && s[i] <= '9')
			i++;
		strcpy(serial_console, "ttyS0,");
		strncpy(serial_console + 6, s, i);
		//prom_printf("Config serial console: %s\n", serial_console);
		puts("Config serial console: %s\n", serial_console);
		console_setup(serial_console, NULL);
	}
#endif

#ifdef CONFIG_REMOTE_DEBUG
	argptr = prom_getcmdline();
	if ((argptr = strstr(argptr, "kgdb=ttyS")) != NULL) {
		int line;
		argptr += strlen("kgdb=ttyS");
		if (*argptr != '0' && *argptr != '1')
			printk("KGDB: Uknown serial line /dev/ttyS%c, "
			       "falling back to /dev/ttyS1\n", *argptr);
		line = *argptr == '0' ? 0 : 1;
		printk("KGDB: Using serial line /dev/ttyS%d for session\n",
		       line ? 1 : 0);

		if (line == 0) {
			rs_kgdb_hook(line);
			putDebugChar = rs_putDebugChar;
			getDebugChar = rs_getDebugChar;
		} else {
			saa9730_kgdb_hook();
			putDebugChar = saa9730_putDebugChar;
			getDebugChar = saa9730_getDebugChar;
			kgdb_on_pci = 1;
		}

		prom_printf
		    ("KGDB: Using serial line /dev/ttyS%d for session, "
		     "please connect your debugger\n", line ? 1 : 0);

		remote_debug = 1;
		/* Breakpoints and stuff are in ev96100_irq_setup() */
	}
#endif
	argptr = prom_getcmdline();

	board_time_init = ev96100_time_init;
	rtc_ops = &ev96100_rtc_ops;
	mips_reboot_setup();

	/*
	 * reassign the start and end from the statically defined start and
	 * end in kernel/resource.
	 */
	ioport_resource.start = GT_PCI_IO_BASE;
	//ioport_resource.end   = GT_PCI_IO_BASE + GT_PCI_IO_SIZE;
	ioport_resource.end = 0xB1FFFFFF;	/* what a hack! */
}
