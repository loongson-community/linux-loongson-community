/*
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
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 * Copyright (C) 2007 MIPS Technologies, Inc.
 *   written by Ralf Baechle
 */
#include <linux/init.h>
#include <linux/console.h>
#include <linux/kernel.h>
#include <linux/serial_reg.h>
#include <asm/io.h>

static inline unsigned int serial_in(int offset)
{
	return inb(0x3f8 + offset);
}

static inline void serial_out(int offset, int value)
{
	outb(value, 0x3f8 + offset);
}

void prom_putchar(char c)
{
	while ((serial_in(UART_LSR) & UART_LSR_THRE) == 0)
		;

	serial_out(UART_TX, c);
}

char getPromChar(void)
{
	while (!(serial_in(UART_LSR) & 1))
		;

	return serial_in(UART_RX);
}

static void mipssim_console_write(struct console *con, const char *s,
	unsigned n)
{
	while (n-- && *s) {
		if (*s == '\n')
			prom_putchar('\r');
		prom_putchar(*s);
		s++;
	}
}

static struct console mipssim_console = {
	.name	= "mipssim",
	.write	= mipssim_console_write,
	.flags	= CON_PRINTBUFFER | CON_BOOT,
	.index	= -1
};

void __init mipssim_setup_console(void)
{
	register_console(&mipssim_console);
}

void __init disable_early_printk(void)
{
	unregister_console(&mipssim_console);
}
